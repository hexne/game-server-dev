/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/02 13:21:20
********************************************************************************/
module;
#include <mysql/mysql.h>
export module database;
import std;
import config;
import log;

class Stmt {
    MYSQL_STMT* stmt_ = nullptr;

public:
    Stmt(MYSQL *connect, const char* sql) {
        stmt_ = mysql_stmt_init(connect);
        if (!stmt_) throw std::runtime_error("mysql_stmt_init failed");

        if (mysql_stmt_prepare(stmt_, sql, std::strlen(sql))) {
            std::string err = mysql_stmt_error(stmt_);
            mysql_stmt_close(stmt_);
            throw std::runtime_error("mysql_stmt_prepare failed: " + err);
        }
    }

    ~Stmt() {
        if (stmt_) mysql_stmt_close(stmt_);
    }

    MYSQL_STMT* get() const { return stmt_; }

    void bind_params(std::vector<MYSQL_BIND>& params) {
        if (mysql_stmt_bind_param(stmt_, params.data())) {
            throw std::runtime_error("bind_param failed");
        }
    }

    void execute() {
        if (mysql_stmt_execute(stmt_)) {
            std::string err = mysql_stmt_error(stmt_);
            throw std::runtime_error("execute failed: " + err);
        }
    }

    std::vector<std::vector<std::string>> fetch_all() {
        MYSQL_RES* meta = mysql_stmt_result_metadata(stmt_);
        if (!meta) return {};

        unsigned int num_fields = mysql_num_fields(meta);

        std::vector<MYSQL_BIND> bind(num_fields);
        std::vector<std::vector<char>> buffers(num_fields, std::vector<char>(256));
        std::vector<unsigned long> lengths(num_fields);

        for (unsigned int i = 0; i < num_fields; ++i) {
            std::memset(&bind[i], 0, sizeof(MYSQL_BIND));
            bind[i].buffer_type   = MYSQL_TYPE_STRING;
            bind[i].buffer        = buffers[i].data();
            bind[i].buffer_length = buffers[i].size();
            bind[i].length        = &lengths[i];
        }

        mysql_stmt_bind_result(stmt_, bind.data());

        std::vector<std::vector<std::string>> rows;

        while (mysql_stmt_fetch(stmt_) == 0) {
            std::vector<std::string> row(num_fields);
            for (unsigned int i = 0; i < num_fields; ++i) {
                row[i] = std::string(buffers[i].data(), lengths[i]);
            }
            rows.push_back(std::move(row));
        }

        mysql_free_result(meta);
        return rows;
    }

};


export class Database {
    std::string host_ = config().database_host;
    int port_ = config().database_port;
    std::string user_ = config().database_user;
    std::string password_ = config().database_password;
    std::string database_ = config().database_name;

    MYSQL* conn_ = nullptr;

public:
    Database() {
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
        if (conn_) {
            mysql_close(conn_);
        }
    }

    // 禁止拷贝，允许移动
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept : conn_(other.conn_) {
        other.conn_ = nullptr;
    }
    Database& operator=(Database&& other) noexcept {
        if (this != &other) {
            if (conn_) mysql_close(conn_);
            conn_ = other.conn_;
            other.conn_ = nullptr;
        }
        return *this;
    }

    MYSQL* get() const { return conn_; }

    MYSQL_STMT* create_stmt(const char* sql) {
        MYSQL_STMT* stmt = mysql_stmt_init(conn_);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");
        if (mysql_stmt_prepare(stmt, sql, std::strlen(sql))) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("mysql_stmt_prepare failed: " + err);
        }
        return stmt;
    }

    template <typename T>
    MYSQL_BIND bind(const T &val) {
        MYSQL_BIND b{};
        std::memset(&b, 0, sizeof(b));

        if constexpr (std::is_same_v<T, std::string>) {
            b.buffer_type   = MYSQL_TYPE_STRING;
            b.buffer        = (void *)(val.c_str());
            b.buffer_length = val.size();
        }
        else if constexpr (std::is_same_v<T, int>) {
            b.buffer_type   = MYSQL_TYPE_LONG;
            b.buffer        = (void*)(&val);
            b.buffer_length = sizeof(val);
        }
        else {
            static_assert(!sizeof(T), "Unsupported type for MYSQL_BIND");
        }

        return b;
    }


    std::vector<std::string> search_user_profile(const std::string& number) {
        const char* sql =
            "SELECT id, name, number, password_hash, create_time, level, exp, rank "
            "FROM users WHERE number=? LIMIT 1";

        Stmt stmt(conn_, sql);

        std::vector<MYSQL_BIND> params;
        params.push_back(bind(number));
        stmt.bind_params(params);

        stmt.execute();
        return stmt.fetch_all().front();
    }


    void update_user_progress(int id, int level, int exp, int rank) {
        const char *sql = "UPDATE users SET level = ?, exp = ?, rank = ? WHERE id = ?";
        Stmt stmt(conn_, sql);
        std::vector<MYSQL_BIND> params;
        params.push_back(bind(level));
        params.push_back(bind(exp));
        params.push_back(bind(rank));
        params.push_back(bind(id));

        stmt.bind_params(params);
        stmt.execute();
    }

    void insert_battle_result(int battle_id, int user_id, const std::string& team, bool winner_is_team_a) {
        const char* sql =
            "INSERT INTO battle_results (battle_id, user_id, team, winner_is_team_a, create_time) "
            "VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)";

        Stmt stmt(conn_, sql);

        int winner = static_cast<int>(winner_is_team_a);
        std::vector<MYSQL_BIND> params;
        params.push_back(bind(battle_id));
        params.push_back(bind(user_id));
        params.push_back(bind(team));
        params.push_back(bind(winner));

        stmt.bind_params(params);
        try {
            stmt.execute();
        }
        catch (...) {
            auto eptr = std::current_exception();
            try {
                if (eptr) std::rethrow_exception(eptr);
            }
            catch (const std::exception& e) {
                Log().push_log(std::format("Fatal error : {}", e.what()));
            }
            catch (...) {
                Log().push_log("Fatal unknown error");
            }
        }
    }

    std::vector<std::vector<std::string>> search_battle(int battle_id) {
        const char* sql =
            "SELECT battle_id, user_id, team, winner_is_team_a, create_time "
            "FROM battle_results WHERE battle_id=?";

        Stmt stmt(conn_, sql);

        std::vector<MYSQL_BIND> params;
        params.push_back(bind(battle_id));

        stmt.bind_params(params);
        stmt.execute();

        return stmt.fetch_all();
    }

    std::vector<std::string> search_battle(int battle_id, int user_id) {
        const char* sql =
            "SELECT battle_id, user_id, team, winner_is_team_a, create_time "
            "FROM battle_results WHERE battle_id=? AND user_id=?";

        Stmt stmt(conn_, sql);

        std::vector<MYSQL_BIND> params;
        params.push_back(bind(battle_id));
        params.push_back(bind(user_id));

        stmt.bind_params(params);
        stmt.execute();

        return stmt.fetch_all().front();
    }

};






