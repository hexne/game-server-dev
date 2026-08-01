/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/02 13:21:20
********************************************************************************/
module;
#include <mysql/mysql.h>
export module database;
import std;


// ===============================
// Result：statement 执行后的内存结果集
// 不再持有 MYSQL_RES*，而是在 fetch 阶段就把每一列拷成 string，
// 这样无论是 mysql_query 还是 mysql_stmt_fetch 都能复用同一套接口，
// 调用方(res[i] / res.empty() / for(auto&row:res))不需要改动。
// ===============================
export class Result {
    std::vector<std::vector<std::string>> rows_;

public:
    Result() = default;
    explicit Result(std::vector<std::vector<std::string>> rows) : rows_(std::move(rows)) {}

    bool empty() const { return rows_.empty(); }
    size_t size() const { return rows_.size(); }

    // 保持和原来一样的用法：res[i] 取第一行第 i 列
    const std::string& operator[](int idx) const {
        return rows_.at(0).at(idx);
    }

    auto begin() const { return rows_.begin(); }
    auto end() const { return rows_.end(); }
};


// ===============================
// PreparedStatement：对 MYSQL_STMT 的 RAII 封装
// 参数一律通过 bind() 传入，绝不拼进 SQL 字符串里
// ===============================
class PreparedStatement {
    MYSQL_STMT* stmt_ = nullptr;
    std::vector<MYSQL_BIND> param_binds_;

    // MYSQL_BIND.buffer 只存指针，真正的数据得单独存一份、
    // 保证从 bind() 调用到 mysql_stmt_execute() 之间不会被析构
    std::deque<long long> int_storage_;
    std::deque<std::string> str_storage_;

public:
    PreparedStatement(MYSQL* conn, const std::string& sql) {
        stmt_ = mysql_stmt_init(conn);
        if (!stmt_)
            throw std::runtime_error("mysql_stmt_init failed");

        if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.size()) != 0) {
            std::string err = mysql_stmt_error(stmt_);
            mysql_stmt_close(stmt_);
            throw std::runtime_error("mysql_stmt_prepare failed: " + err);
        }
    }

    ~PreparedStatement() {
        if (stmt_) mysql_stmt_close(stmt_);
    }

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;
    PreparedStatement(PreparedStatement&&) = default;

    PreparedStatement& bind(int value) {
        int_storage_.push_back(value);

        MYSQL_BIND b{};
        b.buffer_type = MYSQL_TYPE_LONGLONG;
        b.buffer = &int_storage_.back();
        param_binds_.push_back(b);
        return *this;
    }

    PreparedStatement& bind(const std::string& value) {
        str_storage_.push_back(value);

        MYSQL_BIND b{};
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = str_storage_.back().data();
        b.buffer_length = str_storage_.back().size();
        param_binds_.push_back(b);
        return *this;
    }

    // SELECT：执行并把结果整体拷进内存，返回 Result
    Result query() {
        bind_params();
        if (mysql_stmt_execute(stmt_) != 0)
            throw std::runtime_error(mysql_stmt_error(stmt_));

        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt_);
        if (!meta)
            return Result{};

        int field_count = mysql_num_fields(meta);
        mysql_free_result(meta);

        constexpr int kColBufSize = 512;
        std::vector<std::vector<char>> col_buffers(field_count, std::vector<char>(kColBufSize));
        std::vector<unsigned long> col_lengths(field_count);
        std::vector<my_bool> col_is_null(field_count);
        std::vector<MYSQL_BIND> result_binds(field_count);

        for (int i = 0; i < field_count; ++i) {
            result_binds[i] = MYSQL_BIND{};
            result_binds[i].buffer_type = MYSQL_TYPE_STRING;
            result_binds[i].buffer = col_buffers[i].data();
            result_binds[i].buffer_length = kColBufSize;
            result_binds[i].length = &col_lengths[i];
            result_binds[i].is_null = &col_is_null[i];
        }

        mysql_stmt_bind_result(stmt_, result_binds.data());
        mysql_stmt_store_result(stmt_);

        std::vector<std::vector<std::string>> rows;
        while (mysql_stmt_fetch(stmt_) == 0) {
            std::vector<std::string> row;
            row.reserve(field_count);
            for (int i = 0; i < field_count; ++i) {
                if (col_is_null[i])
                    row.emplace_back();
                else
                    row.emplace_back(col_buffers[i].data(), col_lengths[i]);
            }
            rows.push_back(std::move(row));
        }
        return Result(std::move(rows));
    }

    // UPDATE / INSERT / DELETE：只关心是否执行成功，不需要结果集
    void execute() {
        bind_params();
        if (mysql_stmt_execute(stmt_) != 0)
            throw std::runtime_error(mysql_stmt_error(stmt_));
    }

private:
    void bind_params() {
        if (!param_binds_.empty())
            mysql_stmt_bind_param(stmt_, param_binds_.data());
    }
};


export class Database;
class SQLBuilder {
    Database* db_ = nullptr;

    std::vector<std::string> selects_;
    std::string from_;
    std::vector<std::string> wheres_;                       // "column = ?"
    std::vector<std::variant<int, std::string>> params_;    // 与 wheres_ 一一对应的绑定值

public:
    SQLBuilder() = default;
    explicit SQLBuilder(Database* db) : db_(db) {}

    // SELECT
    template <class... Args>
    SQLBuilder& select(Args&&... cols) {
        (selects_.emplace_back(std::forward<Args>(cols)), ...);
        return *this;
    }

    // FROM
    SQLBuilder& from(const std::string& table) {
        from_ = table;
        return *this;
    }

    // WHERE column = <value>
    // value 会作为绑定参数走 prepared statement，不会被拼进 SQL 字符串，
    // 因此这里不再需要（也不允许）传格式化字符串
    template <class T>
    SQLBuilder& where(const std::string& column, T value) {
        wheres_.push_back(column + " = ?");
        params_.emplace_back(std::move(value));
        return *this;
    }

    // 构建 SQL 字符串（此时只有占位符，没有真实数据）
    std::string build() const {
        std::string sql = "SELECT ";

        if (selects_.empty()) {
            sql += "*";
        } else {
            for (size_t i = 0; i < selects_.size(); ++i) {
                if (i) sql += ", ";
                sql += selects_[i];
            }
        }

        sql += " FROM " + from_;

        if (!wheres_.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < wheres_.size(); ++i) {
                if (i) sql += " AND ";
                sql += wheres_[i];
            }
        }

        return sql;
    }

    // 执行 SQL
    Result exec();
};


// ===============================
// Database：你要求的最终封装
// ===============================
export class Database {
    std::string host_ = "127.0.0.1";
    int port_ = 3306;
    std::string user_{};
    std::string password_{};
    std::string database_{};

    MYSQL* conn_ = nullptr;

public:
    Database(std::string user, std::string password, std::string database)
        : user_(std::move(user)),
          password_(std::move(password)),
          database_(std::move(database))
    {
        conn_ = mysql_init(nullptr);
        if (!conn_) {
            throw std::runtime_error("mysql_init failed");
        }

        if (!mysql_real_connect(
                conn_,
                host_.c_str(),
                user_.c_str(),
                password_.c_str(),
                database_.c_str(),
                port_,
                nullptr,
                0))
        {
            std::string err = mysql_error(conn_);
            mysql_close(conn_);
            throw std::runtime_error("MySQL connect failed: " + err);
        }
    }

    ~Database() {
        if (conn_) mysql_close(conn_);
    }

    // 只允许通过 prepared statement 访问数据库，
    // 不再对外暴露"传原始 SQL 字符串"的 query 接口，从源头堵住拼接注入的可能性
    PreparedStatement prepare(const std::string& sql) {
        return PreparedStatement(conn_, sql);
    }

    SQLBuilder builder_{this};
    auto operator -> () {
        builder_ = SQLBuilder(this);
        return &builder_;
    }
};

Result SQLBuilder::exec() {
    auto stmt = db_->prepare(build());
    for (auto& p : params_) {
        std::visit([&stmt](auto&& v) { stmt.bind(v); }, p);
    }
    return stmt.query();
}

export std::string search_user_profile(Database &db, std::string number) {
    auto res = db->select("id", "name", "number", "password_hash", "create_time", "level", "exp", "rank")
        .from("users")
        .where("number", number)
        .exec();

    if (res.empty())
        return {};

    return std::format("{}|{}|{}|{}|{}|{}|{}|{}",
        res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7]);
}

export bool update_user_progress(Database &db, int id, int level, int exp, int rank) {
    try {
        auto stmt = db.prepare("UPDATE users SET level = ?, exp = ?, rank = ? WHERE id = ?");
        stmt.bind(level).bind(exp).bind(rank).bind(id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}
