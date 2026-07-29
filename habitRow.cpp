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
        int square_color = data[i] - '0';
        int square_w = 200 + i*57;
        int square_h = this->height_loc;
        HabitSquare square(square_w, square_h, square_color, i); // pass in each squares height and location.
        this->row.push_back(square);
    }
}

void HabitRow::draw(sf::RenderWindow& window){
    for(int i =0; i < this->row.size(); i++){
        row[i].draw(window);
    }
}

void HabitRow::onClick(sf::Vector2f mousePos, sf::Mouse::Button clickType){
    for(int i =0; i < this->row.size(); i++){
        row[i].onClick(mousePos, clickType);
    }
}



std::string HabitRow::rowData(){
    std::string dataString ="";
    for(int i =0; i < this->row.size(); i++){
        dataString += std::to_string(row[i].getClicked());
    }
    std::cout << dataString << std::endl;
    return dataString;
}