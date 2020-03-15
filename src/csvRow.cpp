#include "csvRow.h"


/* CsvColumn implementation */

CsvColumn::CsvColumn(std::string name, unsigned int order){
    this->name = name;
    this->order = order;
}

std::string CsvColumn::getName(){
    return name;
}

unsigned int CsvColumn::getOrder(){
    return order;
}


/* CsvColumns implementation */

CsvColumns::CsvColumns(){
    this->size = 0;
};

CsvColumns::~CsvColumns(){
    while(!columns.empty()){
        delete columns.back();
        columns.pop_back();
    }
};

CsvColumn* CsvColumns::add(std::string name){
    CsvColumn* newColumn = new CsvColumn(name, size);
    if(newColumn){
        ++size;
        columns.push_back(newColumn);
        return columns.back();
    }
    return NULL;
}

unsigned int CsvColumns::getSize(){
   return size; 
}


CsvColumn* CsvColumns::getColumn(unsigned int index){
    if(index < size){
        return columns[index];
    }
    return NULL;
}


/* CsvRow implementation */

void CsvRow::set(CsvColumn* column, std::string data){
    const unsigned int order = column->getOrder();
    data = makeCsvSafe(data);
    data.push_back(',');
    if(order < row.size()){
        row[order] = data;
    }
    else{
        while(row.size() < order){
            row.push_back(",");
        }
        row.push_back(data);
    }
};

std::string CsvRow::getRow(){
    std::string line;
    for(std::string str: row){
        line.append(str);
    }
    line.pop_back();
    line.push_back('\n');
    return line;
}

void CsvRow::write(FILE *fp){
   fprintf(fp, "%s", (getRow()).c_str());
}

void CsvRow::clean(){
    while(!row.empty()) 
        row.pop_back();
}


std::string makeCsvSafe(std::string unsafe){
    //Each of the embedded double-quote characters 
    //must be represented by a pair of double-quote characters
    int first = 0, last = unsafe.size();
    //find pairs of double-quote and quote them
    while((first = unsafe.find_first_of('"', first)) >= 0 && 
            (last = unsafe.find_last_of('"', last)) >= 0 &&
            first < last){
        unsafe.insert(first, "\"");
        unsafe.insert(last + 1, "\"");
        first+=2; //after insert index of founded character is incremented, therefore +2
        last--;
    }
    //Fields with embedded commas or line breaks characters must be quoted
    int comma, whitespace; 
    if((comma = unsafe.find_first_of(',')) >= 0 || 
            (whitespace = unsafe.find_first_of(' ')) >= 0){
        unsafe.insert(0, "\"");
        unsafe.push_back('"');
    }
    return unsafe;
}

