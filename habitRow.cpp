#include "habitRow.hpp"
#include <iostream>

HabitRow::HabitRow(int h_loc)
:
height_loc(h_loc)
{}

void HabitRow::initialize_row(){
    for(int i =0; i < this->columns; i++){
        int square_w = 200 + i*57;
        int square_h = this->height_loc;
        HabitSquare square(square_w, square_h); // pass in each squares height and location.
        this->row.push_back(square);
    }
}
void HabitRow::initialize_row(std::string data){
    for(int i =0; i < this->columns; i++){
        bool square_color = (data[i] == '1');
        int square_w = 200 + i*57;
        int square_h = this->height_loc;
        HabitSquare square(square_w, square_h, square_color); // pass in each squares height and location.
        this->row.push_back(square);
    }
}

void HabitRow::draw(sf::RenderWindow& window){
    for(int i =0; i < this->row.size(); i++){
        row[i].draw(window);
    }
}

void HabitRow::onClick(sf::Vector2f mousePos){
    for(int i =0; i < this->row.size(); i++){
        row[i].onClick(mousePos);
    }
}

std::string HabitRow::rowData(){
    std::string dataString ="";
    for(int i =0; i < this->row.size(); i++){
        dataString += row[i].getClicked() ? '1' : '0';
    }
    std::cout << dataString << std::endl;
    return dataString;
}