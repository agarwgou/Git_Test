//
//  testing.cpp
//  Project_QF633_source
//
//  Created by Gourav Agarwal on 18/6/23.
//


#include <iostream>
#include <fstream>
#include <string>
using namespace std;
int main_1(int argc, char** argv){
    
    std::cout<<"Setting file: "<<argv[1]<<std::endl;
    const char* ticker_filename = argv[1];
    
    ifstream infilestream;
    string line;
    int n=0;
    
    //"TestData/20220515_BTC_Ticker.csv"
    cout<<"input: "<<argv[1]<<endl;
    infilestream.open(ticker_filename); //open a file to perform read operation using file object
    cout<<"Result"<<endl;
    cout<<infilestream.is_open()<<endl;
    if (infilestream.is_open()) {
        std::string line;
        while (std::getline(infilestream, line) && n<2) {
            // using printf() in all tests for consistency
            printf("%s", line.c_str());
            n += 1;
        }
        //while(infilestream)
        //{
        //  std::getline(infilestream, line);
        //  cout<<line<<"\n";
        //}
        infilestream.close();
    } else {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
   }
    
    return 0;
}

/*
 
 #include <iostream>
 #include <fstream>
 #include <sstream>
 #include <string>
 #include <vector>

int main() {
    std::ifstream file("hi.txt"); // Replace "filename.csv" with the path to your CSV file

    if (file.is_open()) {
        std::string line;

        // Read and process each line from the CSV file
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> values;

            // Parse the line using comma as the delimiter
            while (std::getline(iss, token, ',')) {
                values.push_back(token);
            }

            // Process the values as needed
            // Example: Print the values
            for (const auto& value : values) {
                std::cout << value << " ";
            }
            std::cout << std::endl;
        }

        file.close();
    } else {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
    }

    return 0;
}

*/

/*
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
    const char* ticker_filename = argv[1];
    std::ifstream file(ticker_filename); // Replace "filename.txt" with the path to your file
    
    std::cout<<ticker_filename<<std::endl;
    if (file.is_open()) {
        std::string line;

        // Read and print each line from the file
        while (std::getline(file, line)) {
            std::cout << line << std::endl;
        }

        file.close();
    } else {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
    }

    return 0;
}
*/


