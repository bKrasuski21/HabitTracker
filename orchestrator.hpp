#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "habitSquare.hpp"
#include "habitRow.hpp"
#include "nameBlock.hpp"


class Orchestrator
{
    // handles creation and organization of habitRows
    public: 
    Orchestrator();
    void getHabits();
    void run();
    void initialize_grid();
    void draw();
    void build_habit_column();
    void onClick(sf::Vector2f mousePos);
    void readMemoryFile();
    void writeMemoryFile();
    
    private:
    std::vector<HabitRow> grid;
    std::vector<std::string> habits;
    std::vector<NameBlock> name_column;

    unsigned int window_width = 1920;
    unsigned int window_length = 1480;
    int habit_num;
    sf::RenderWindow window;
    

    
};