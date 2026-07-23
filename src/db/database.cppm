/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/02 13:21:20
********************************************************************************/
module;
#include <mysql/mysql.h>
export module database;
import std;


// ===============================
// Row：对 MYSQL_ROW 的轻量封装
// ===============================
class Row {
    MYSQL_ROW row_;

public:
    Row(MYSQL_ROW r) : row_(r) {}

    const char* operator[](int idx) const {
        return row_[idx];
    }

    int get_int(int idx) const {
        return std::stoi(row_[idx]);
    }

    std::string get_string(int idx) const {
        return row_[idx];
    }
};


// ===============================
// ResultIterator：自定义迭代器
// ===============================
class ResultIterator {
    MYSQL_RES* res_;
    MYSQL_ROW row_;

public:
    ResultIterator(MYSQL_RES* res, MYSQL_ROW row)
        : res_(res), row_(row) {}

    Row operator*() const {
        return Row(row_);
    }

    ResultIterator& operator++() {
        row_ = mysql_fetch_row(res_);
        return *this;
    }

    bool operator!=(const ResultIterator& other) const {
        return row_ != other.row_;
    }
};


// ===============================
// Result：封装 MYSQL_RES，提供 begin/end
// ===============================
class Result {
    MYSQL_RES* res_;
    bool valid_ = false;
    unsigned long long row_count_ = 0;
    MYSQL_ROW row_;
public:
    explicit Result(MYSQL_RES* r)
        : res_(r),
          valid_(r != nullptr),
          row_count_(r ? mysql_num_rows(r) : 0) {
        row_ = mysql_fetch_row(res_);
    }

    ~Result() {
        if (res_) mysql_free_result(res_);
    }

    bool valid() const { return valid_; }
    bool empty() const { return row_count_ == 0; }
    unsigned long long size() const { return row_count_; }
    auto operator[] (int index) const {
        return row_[index];
    }
    ResultIterator begin() {
        return ResultIterator(res_, mysql_fetch_row(res_));
    }

    ResultIterator end() {
        return ResultIterator(res_, nullptr);
    }
};




export class Database;
class SQLBuilder {
    Database* db_ = nullptr;

    std::vector<std::string> selects_;
    std::string from_;
    std::vector<std::string> wheres_;

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

    // WHERE
    template <class... Args>
    SQLBuilder& where(const std::string& fmt, Args&&... args) {
        auto cond = std::vformat(fmt.c_str(), std::make_format_args(args...));
        wheres_.push_back(std::move(cond));
        return *this;
    }

    // 构建 SQL 字符串
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

    // ⭐ 你要求的 query：返回可迭代对象
    Result query(const std::string& sql) {
        if (mysql_query(conn_, sql.c_str()) != 0) {
            throw std::runtime_error(mysql_error(conn_));
        }

        MYSQL_RES* res = mysql_store_result(conn_);
        if (!res) {
            throw std::runtime_error("mysql_store_result failed");
        }
        return Result(res);
    }

    SQLBuilder builder_{this};
    auto operator -> () {
        builder_ = SQLBuilder(this);
        return &builder_;
    }
};

Result SQLBuilder::exec() {
    return db_->query(build());
}

export std::string search_user_profile(Database &db, std::string number) {
    auto res = db->select("id", "name", "number", "password_hash", "create_time", "level", "rank")
        .from("users")
        .where("number = '{}'", number)
        .exec();

    if (res.empty())
        return {};

    return std::format("{}|{}|{}|{}|{}|{}|{}",
        res[0], res[1], res[2], res[3], res[4], res[5], res[6]);
}