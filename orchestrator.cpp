#include "orchestrator.hpp"
#include <iostream>
#include <optional>
#include <filesystem>
#include <fstream>

Orchestrator::Orchestrator()
: window(sf::VideoMode({window_width, window_length}
), "habit Tracker")
{
    
}


void Orchestrator::run(){
    // change if read or new memory file here instead of in get habits 
    std::cout << "create new habit memory sheet? : (yes|no)" << std::endl;
    std::string response;
    std::cin >> response;
    if(response == "no") { 
        this->readMemoryFile();
    }else {
        this->getHabits();
        this->initialize_grid();
    }
    
    sf::Event event;

    while(this->window.isOpen()){
        while(window.pollEvent(event)){
            if(event.type == sf::Event::Closed){
                writeMemoryFile();
                this->window.close();
            }  
            if(event.type==sf::Event::MouseButtonPressed){
                
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                onClick(mousePos);
            }
        }
        window.clear();
        this->draw();
        window.display();
    }
}

void Orchestrator::getHabits(){
    int tracked;
    
    std::cout << "how many habits would you like" << std::endl;
    std::cin >> tracked;
    this->habit_num = tracked;

    for(int i = 0; i < this->habit_num; i++){
        std::cout << "Provide name of your habit!" << std::endl;
        std::string curr_habit;

        std::cin >> curr_habit;
        this->habits.push_back(curr_habit);

        }

}



void Orchestrator::readMemoryFile(){
    std::ifstream file("./data/mem.txt");
    std::string line;
    int count = 0;
    while(std::getline(file,line)){
        // read the first line, pushback name to habits 
        // for now just do the above ^ 
        // next: pass into initialize row the data string the row will deal with it .
        std::string data = "";
        for(int i = 0; i < line.size(); i++){
            if(line[i] == ','){
                habits.push_back(line.substr(0,i));
                data = line.substr(i+1, line.size()-i+1);
                std::cout << "habit: " << line.substr(0,i+1) << " data " << data << std::endl;
            }
        }
        
        int row_height = count*57;
        HabitRow row(row_height);
        row.initialize_row(data);
        this->grid.push_back(row);
        count++;
    }
    for(int i = 0; i <this->habits.size(); i++){
        int height = i * 57;
        NameBlock habitBlock(habits[i], height);
        this->name_column.push_back(habitBlock);
    }

    for(int i =0; i < this->name_column.size(); i++){
        this->name_column[i].initialize();
    }


}



void Orchestrator::writeMemoryFile(){
    std::ofstream file("./data/mem.txt");

    for(int i =0; i < grid.size(); i++){
        std::string builderString = "";
        builderString += name_column[i].getString();
        builderString += ",";
        builderString += grid[i].rowData();
        file << builderString << "\n";

    }
    file.close();
    std::cout <<"memory written" << std::endl;
}

void Orchestrator::initialize_grid(){
    for(int i =0; i < this->habits.size(); i++){
        int row_height = i * 57;
        HabitRow row(row_height);
        row.initialize_row();
        this->grid.push_back(row);
    }

    for(int i = 0; i <this->habits.size(); i++){
        int height = i * 57;
        NameBlock habitBlock(habits[i], height);
        this->name_column.push_back(habitBlock);
    }

    for(int i =0; i < this->name_column.size(); i++){
        this->name_column[i].initialize();
    }

}

void Orchestrator::draw(){
    for(int i =0; i < grid.size(); i++){
        grid[i].draw(this->window);
    }
    for(int i =0; i <this->name_column.size(); i++){
        name_column[i].draw(this->window);
    }
}

void Orchestrator::onClick(sf::Vector2f mousePos){
    std::cout << "Register click" << std::endl;
    for(int i =0; i < grid.size(); i++){
        grid[i].onClick(mousePos);
    }
}
