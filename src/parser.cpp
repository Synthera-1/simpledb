#include "parser.h"
#include <stdexcept>

// The parser: turns tokens into a Statement (aka “what SQL wants to do”).
Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}


Token Parser::current() { return tokens[pos]; }
Token Parser::peek()    { return tokens[pos + 1]; }

Token Parser::consume() {
    Token t = tokens[pos];
    pos++;
    return t;
}

Token Parser::expect(TokenType type, const std::string& errorMsg) {
    if (current().type != type) {
        throw std::runtime_error(errorMsg + " (got '" + current().value + "')");
    }
    return consume();
}

bool Parser::check(TokenType type) { return current().type == type; }

bool Parser::match(TokenType type) {
    if (check(type)) { consume(); return true; }
    return false;
}

//the entry point. looks at the first token and routes it to the right parse methodo. 
Statement Parser::parse() {
    switch (current().type) {
        case TokenType::SELECT: return parseSelect();
        case TokenType::INSERT: return parseInsert();
        case TokenType::UPDATE: return parseUpdate();
        case TokenType::DELETE: return parseDelete();
        case TokenType::CREATE: return parseCreateTable();
        case TokenType::DROP:   return parseDropTable();
        default:
            throw std::runtime_error("Unknown statement: '" + current().value + "'");
    }
}

// CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, age INT)
Statement Parser::parseCreateTable() {
    Statement stmt;
    stmt.type = StatementType::CREATE_TABLE;
    consume(); // CREATE
    expect(TokenType::TABLE, "Expected TABLE after CREATE");
    stmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
    expect(TokenType::LPAREN, "Expected '('");
    while (!check(TokenType::RPAREN)) {
        stmt.columns.push_back(parseColumnDef());
        if (!match(TokenType::COMMA)) break;
    }
    expect(TokenType::RPAREN, "Expected ')'");
    return stmt;
}

//reads a column definition. so it reads the name, then type, then any contraints. and whats so cool about this thing i made
//is that it can handle multiple constraints on one column using the loop dy loop.
ColumnDef Parser::parseColumnDef() {
    ColumnDef col;
    col.name = expect(TokenType::IDENTIFIER, "Expected column name").value;
    // Parse data type
    if (check(TokenType::INT))        { col.type = DataType::INT;   consume(); }
    else if (check(TokenType::FLOAT)) { col.type = DataType::FLOAT; consume(); }
    else if (check(TokenType::TEXT))  { col.type = DataType::TEXT;  consume(); }
    else throw std::runtime_error("Expected data type for column '" + col.name + "'");
    // Parse constraints
    while (check(TokenType::PRIMARY) || check(TokenType::NOT) || check(TokenType::UNIQUE)) {
        if (match(TokenType::PRIMARY)) {
            expect(TokenType::KEY, "Expected KEY after PRIMARY");
            col.primaryKey = true;
            col.notNull = true;
        } else if (match(TokenType::NOT)) {
            expect(TokenType::NULL_VAL, "Expected NULL after NOT");
            col.notNull = true;
        } else if (match(TokenType::UNIQUE)) {
            col.unique = true;
        }
    }
    return col;
}

// DROP TABLE users
Statement Parser::parseDropTable() {
    Statement stmt;
    stmt.type = StatementType::DROP_TABLE;
    consume(); // DROP
    expect(TokenType::TABLE, "Expected TABLE after DROP");
    stmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
    return stmt;
}

// INSERT INTO users VALUES (stuff). it reads values seperated by the "." these commas until it hits a wall. ")".
Statement Parser::parseInsert() {
    Statement stmt;
    stmt.type = StatementType::INSERT;
    consume(); // INSERT
    expect(TokenType::INTO, "Expected INTO after INSERT");
    stmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
    expect(TokenType::VALUES, "Expected VALUES");
    expect(TokenType::LPAREN, "Expected '('");
    while (!check(TokenType::RPAREN)) {
        if (check(TokenType::STRING_LITERAL) ||
            check(TokenType::INTEGER_LITERAL) ||
            check(TokenType::FLOAT_LITERAL)) {
            stmt.values.push_back(consume().value);
        } else if (check(TokenType::NULL_VAL)) {
            stmt.values.push_back("NULL");
            consume();
        } else {
            throw std::runtime_error("Expected value in INSERT");
        }
        if (!match(TokenType::COMMA)) break;
    }
    expect(TokenType::RPAREN, "Expected ')'");
    return stmt;
}

// this is most complex one because SELECT has so much stuff and clauses. my code handles it in order like this: 
// SELECT -> columns/aggregates -> FROM -> JOIN? -> WHERE? -> ORDER BY? -> LIMIT?
//im so proud of this section
Statement Parser::parseSelect() {
    Statement stmt;
    stmt.type = StatementType::SELECT;
    consume(); // SELECT

    // Parse columns or aggregates
    if (match(TokenType::STAR)) {
        // SELECT * — selectColumns stays empty
    } else {
        while (true) {
            // Aggregate function?
            if (check(TokenType::COUNT) || check(TokenType::SUM) ||
                check(TokenType::AVG)   || check(TokenType::MIN) ||
                check(TokenType::MAX)) {
                AggregateFunc agg;
                agg.func = consume().value;
                expect(TokenType::LPAREN, "Expected '(' after aggregate function");
                if (match(TokenType::STAR)) {
                    agg.column = "*";
                } else {
                    agg.column = expect(TokenType::IDENTIFIER, "Expected column name").value;
                }
                expect(TokenType::RPAREN, "Expected ')'");
                stmt.aggregates.push_back(agg);
            } else {
                stmt.selectColumns.push_back(
                    expect(TokenType::IDENTIFIER, "Expected column name").value
                );
            }
            if (!match(TokenType::COMMA)) break;
        }
    }

    expect(TokenType::FROM, "Expected FROM");
    stmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;

    // JOIN?
    if (check(TokenType::INNER) || check(TokenType::LEFT) || check(TokenType::JOIN)) {
        if (match(TokenType::LEFT)) stmt.isLeftJoin = true;
        match(TokenType::INNER);
        expect(TokenType::JOIN, "Expected JOIN");
        stmt.joinTable = expect(TokenType::IDENTIFIER, "Expected table name").value;
        expect(TokenType::ON, "Expected ON");
        stmt.joinLeftCol  = expect(TokenType::IDENTIFIER, "Expected column").value;
        expect(TokenType::DOT, "Expected '.'");
        stmt.joinLeftCol  += "." + expect(TokenType::IDENTIFIER, "Expected column").value;
        expect(TokenType::EQ, "Expected '='");
        stmt.joinRightCol  = expect(TokenType::IDENTIFIER, "Expected column").value;
        expect(TokenType::DOT, "Expected '.'");
        stmt.joinRightCol += "." + expect(TokenType::IDENTIFIER, "Expected column").value;
    }

    // WHERE?
    if (match(TokenType::WHERE)) {
        stmt.conditions = parseWhere();
    }

    // ORDER BY?
    if (check(TokenType::ORDER)) {
        consume();
        expect(TokenType::BY, "Expected BY after ORDER");
        stmt.orderBy.column = expect(TokenType::IDENTIFIER, "Expected column name").value;
        stmt.hasOrderBy = true;
        if (match(TokenType::DESC)) stmt.orderBy.ascending = false;
        else match(TokenType::ASC);
    }

    // LIMIT?
    if (match(TokenType::LIMIT)) {
        stmt.limit = std::stoi(expect(TokenType::INTEGER_LITERAL, "Expected number after LIMIT").value);
    }

    return stmt;
}

// UPDATE users SET name = 'john python' WHERE id = 1
Statement Parser::parseUpdate() {
    Statement stmt;
    stmt.type = StatementType::UPDATE;
    consume(); // UPDATE
    stmt.tableName   = expect(TokenType::IDENTIFIER, "Expected table name").value;
    expect(TokenType::SET, "Expected SET");
    stmt.updateColumn = expect(TokenType::IDENTIFIER, "Expected column name").value;
    expect(TokenType::EQ, "Expected '='");
    stmt.updateValue  = consume().value;
    if (match(TokenType::WHERE)) {
        stmt.conditions = parseWhere();
    }
    return stmt;
}

// DELETE FROM users WHERE id = 1
Statement Parser::parseDelete() {
    Statement stmt;
    stmt.type = StatementType::DELETE;
    consume(); // DELETE
    expect(TokenType::FROM, "Expected FROM after DELETE");
    stmt.tableName = expect(TokenType::IDENTIFIER, "Expected table name").value;
    if (match(TokenType::WHERE)) {
        stmt.conditions = parseWhere();
    }
    return stmt;
}

std::vector<Condition> Parser::parseWhere() {
    std::vector<Condition> conditions;
    conditions.push_back(parseCondition());
    while (check(TokenType::AND) || check(TokenType::OR)) {
        std::string logic = consume().value;
        Condition cond = parseCondition();
        cond.logic = logic;
        conditions.push_back(cond);
    }
    return conditions;
}

Condition Parser::parseCondition() {
    Condition cond;
    cond.column = expect(TokenType::IDENTIFIER, "Expected column name").value;
    switch (current().type) {
        case TokenType::EQ:  cond.op = "=";  break;
        case TokenType::NEQ: cond.op = "!="; break;
        case TokenType::LT:  cond.op = "<";  break;
        case TokenType::GT:  cond.op = ">";  break;
        case TokenType::LTE: cond.op = "<="; break;
        case TokenType::GTE: cond.op = ">="; break;
        default: throw std::runtime_error("Expected operator in WHERE clause");
    }
    consume();
    cond.value = consume().value;
    return cond;
}
