#ifndef CSVROW
#define CSVROW

#include <iostream>
#include <vector>

std::string csvEscape(std::string unsafe);

class CsvRow;

class CsvColumn {
private:
    std::string header;
    unsigned int order;

public:
    CsvColumn(std::string header, unsigned int order);

    std::string getHeader();

    unsigned int getOrder();
};

class CsvColumns {
private:
    std::vector<CsvColumn *> columns;

public:
    ~CsvColumns();

    CsvColumn *add(std::string name);

    void setHeader(CsvRow &row);
};

class CsvRow {
private:
    std::vector<std::string> row;

public:
    void set(CsvColumn *column, double data);
    void set(CsvColumn *column, std::string data);

    std::string getValue(CsvColumn *column);

    std::string toString();

    void write(FILE *fp);

    void clear();

    bool empty();
};
#endif
