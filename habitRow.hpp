#pragma once
#include <SFML/Graphics.hpp>
#include "habitSquare.hpp"
#include <string>
#include <vector>

class HabitRow
{
    public:
    HabitRow(int h_loc);  // pass in each rows height
    void initialize_row();
    void initialize_row(std::string data);
    void draw(sf::RenderWindow& window);
    void onClick(sf::Vector2f mousPos);
    std::string rowData();
    
    private:
    int width_loc = 200;
    int height_loc;
    std::vector<HabitSquare> row;
    int row_height;
    int columns = 30;

};