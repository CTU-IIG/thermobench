#ifndef CSVROW
#define CSVROW

#include <iostream>
#include <vector>

std::string makeCsvSafe(std::string unsafe);

class CsvColumn{
    private:
        std::string name;
        unsigned int order;

    public:
        CsvColumn(std::string name, unsigned int order);

        std::string getName();

        unsigned int getOrder();
};

class CsvColumns{
    private:
        std::vector<CsvColumn*> columns;
        unsigned int size;
    
    public:
        CsvColumns();

        ~CsvColumns();

        CsvColumn* add(std::string name);
        
        unsigned int getSize();

        CsvColumn* getColumn(unsigned int index);
};

class CsvRow{
    private:
        std::vector<std::string> row;

    public:
        void set(CsvColumn* column, std::string data);

        std::string getRow();

        void write(FILE *fp);

        void clean();
};
#endif
