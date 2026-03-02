#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../include/httplib.h"

using namespace httplib;
using namespace std;

bool sendMessage(const string& server, int port, const string& username, const string& message) {
    Client cli(server, port);
    string json = "{\"username\":\"" + username + "\",\"message\":\"" + message + "\"}";
    auto res = cli.Post("/api/send", json, "application/json");
    return (res && res->status == 200);
}

void receiveMessages(const string& server, int port, bool& running) {
    Client cli(server, port);
    
    while(running) {
        auto res = cli.Get("/api/messages");
        
        if(res && res->status == 200) {
            system("cls");
            cout << "=== Чат ===" << endl;
            cout << "Сервер: " << server << ":" << port << endl;
            cout << "Команды: quit - выход, clear - очистить" << endl;
            cout << "========================" << endl << endl;
            
            string body = res->body;
            size_t pos = 0;
            
            while((pos = body.find("{\"username\":\"", pos)) != string::npos) {
                pos += 13;
                size_t usernameEnd = body.find("\"", pos);
                string username = body.substr(pos, usernameEnd - pos);
                
                size_t contentPos = body.find("\"content\":\"", usernameEnd);
                if(contentPos != string::npos) {
                    contentPos += 10;
                    size_t contentEnd = body.find("\"", contentPos);
                    string content = body.substr(contentPos, contentEnd - contentPos);
                    
                    size_t timePos = body.find("\"timestamp\":\"", contentEnd);
                    if(timePos != string::npos) {
                        timePos += 12;
                        size_t timeEnd = body.find("\"", timePos);
                        string timestamp = body.substr(timePos, timeEnd - timePos);
                        
                        cout << "[" << timestamp << "] " << username << ": " << content << endl;
                    }
                }
                pos = body.find("}", pos) + 1;
            }
        }
        this_thread::sleep_for(chrono::seconds(2));
    }
}

void clearChat(const string& server, int port) {
    Client cli(server, port);
    auto res = cli.Post("/api/clear");
    if(res && res->status == 200) {
        cout << "Чат очищен" << endl;
    }
}

int main() {
    std::setlocale(LC_ALL, "Russian");
    string server = "localhost";
    int port = 8080;
    string username;
    bool running = true;
    
    cout << "Введите ваше имя: ";
    getline(cin, username);
    
    if(username.empty()) username = "Аноним";
    
    cout << "Подключение к " << server << ":" << port << "..." << endl;
    
    Client cli(server, port);
    auto res = cli.Get("/");
    if(!res) {
        cout << "Ошибка: Сервер не доступен!" << endl;
        cout << "Запустите сервер сначала" << endl;
        system("pause");
        return 1;
    }
    
    cout << "Подключено успешно!" << endl;
    cout << "Введите 'quit' для выхода, 'clear' для очистки" << endl << endl;
    
    thread receiver(receiveMessages, server, port, ref(running));
    
    string input;
    while(running) {
        getline(cin, input);
        
        if(input == "quit") {
            running = false;
            break;
        }
        else if(input == "clear") {
            clearChat(server, port);
        }
        else if(!input.empty()) {
            if(sendMessage(server, port, username, input)) {
                cout << "[OK] Сообщение отправлено" << endl;
            } else {
                cout << "[ERR] Ошибка отправки" << endl;
            }
        }
    }
    
    receiver.join();
    return 0;
}
