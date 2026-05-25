#pragma once
#include <string>
#include <vector>
#include <memory>
#include "token.h"
#include "datatype.h"

// what types we like to see (if we dont like to see it it doesnt work. haha. it just means what types we support).
enum class StatementType {
    CREATE_TABLE,
    DROP_TABLE,
    INSERT,
    SELECT,
    UPDATE,
    DELETE
};

// where conditions.
struct Condition {
    std::string column;
    std::string op;      // =, !=, <, >, <=, >=
    std::string value;
    std::string logic;   // AND, OR (empty for first condition)
};

// orders by the clause. (not the santa clause, the SQL clause.)
struct OrderBy {
    std::string column;
    bool ascending = true;
};

// aggreaggregate functions and all that stuff.
struct AggregateFunc {
    std::string func;    // COUNT, SUM, AVG, MIN, MAX
    std::string column;  // * for COUNT(*)
};

// one big struct! it has everything about any sql statement. 
struct Statement {
    StatementType type;

    // CREATE TABLE
    std::string tableName;
    std::vector<ColumnDef> columns;

    // INSERT
    std::vector<std::string> values;

    // SELECT
    std::vector<std::string> selectColumns;  // empty = SELECT *
    std::vector<AggregateFunc> aggregates;
    std::vector<Condition> conditions;       // WHERE
    OrderBy orderBy;
    bool hasOrderBy = false;
    int limit = -1;                          // -1 = no limit

    // JOIN
    std::string joinTable;
    std::string joinLeftCol;
    std::string joinRightCol;
    bool isLeftJoin = false;

    // UPDATE
    std::string updateColumn;
    std::string updateValue;
};

//its like the parser's navigation tools. like its compass and map. works exactly like the lexer current and peek and
//advance but it works on tokens not characters. expect is pretty important you know. we use it when a token is required.
class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    Statement parse();

//uno methedo per statement type. it looks at the first token and thinks real hard on which to call. 
private:
    std::vector<Token> tokens;
    int pos;

    Token current();
    Token peek();
    Token consume();
    Token expect(TokenType type, const std::string& errorMsg);
    bool check(TokenType type);
    bool match(TokenType type);

    Statement parseCreateTable();
    Statement parseDropTable();
    Statement parseInsert();
    Statement parseSelect();
    Statement parseUpdate();
    Statement parseDelete();

    std::vector<Condition> parseWhere();
    Condition parseCondition();
    ColumnDef parseColumnDef();
};
