#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include "../include/httplib.h"

using namespace httplib;
using namespace std;

struct Message {
    string username;
    string content;
    string timestamp;
};

vector<Message> chatMessages;
mutex messagesMutex;

string getCurrentTime() {
    time_t now = time(nullptr);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

int main() {
    std::setlocale(LC_ALL, "Russian");
    Server svr;
    
    cout << "==================================" << endl;
    cout << "Сервер чата запущен!" << endl;
    cout << "Адрес: http://localhost:8080" << endl;
    cout << "Для остановки нажмите Ctrl+C" << endl;
    cout << "==================================" << endl;
    
    svr.Get("/", [](const Request& req, Response& res) {
        string html = "<!DOCTYPE html><html><head><title>Чат</title>";
        html += "<meta charset='UTF-8'>";
        html += "<style>";
        html += "body { font-family: Arial; margin: 20px; }";
        html += "#messages { height: 400px; overflow-y: scroll; border: 1px solid #ccc; padding: 10px; }";
        html += ".message { margin-bottom: 10px; }";
        html += ".username { font-weight: bold; color: #2196F3; }";
        html += ".time { color: #999; font-size: 0.8em; }";
        html += "</style></head><body>";
        html += "<h1>Чат</h1>";
        html += "<div id='messages'></div>";
        html += "<input type='text' id='username' placeholder='Имя' value='Аноним'>";
        html += "<input type='text' id='message' placeholder='Сообщение'>";
        html += "<button onclick='sendMessage()'>Отправить</button>";
        html += "<script>";
        html += "function loadMessages(){";
        html += "fetch('/api/messages').then(r=>r.json()).then(m=>{";
        html += "let html='';";
        html += "m.forEach(msg=>{";
        html += "html+='<div class=\"message\">';";
        html += "html+='<span class=\"username\">'+msg.username+'</span>';";
        html += "html+='<span class=\"time\">['+msg.timestamp+']</span>';";
        html += "html+='<div>'+msg.content+'</div>';";
        html += "html+='</div>';";
        html += "});";
        html += "document.getElementById('messages').innerHTML=html;";
        html += "});}";
        html += "function sendMessage(){";
        html += "fetch('/api/send',{";
        html += "method:'POST',";
        html += "headers:{'Content-Type':'application/json'},";
        html += "body:JSON.stringify({";
        html += "username:document.getElementById('username').value,";
        html += "message:document.getElementById('message').value";
        html += "})}).then(()=>{";
        html += "document.getElementById('message').value='';";
        html += "loadMessages();";
        html += "});}";
        html += "setInterval(loadMessages,2000);";
        html += "loadMessages();";
        html += "</script></body></html>";
        res.set_content(html, "text/html");
    });
    
    svr.Get("/api/messages", [](const Request& req, Response& res) {
        lock_guard<mutex> lock(messagesMutex);
        string json = "[";
        for(size_t i = 0; i < chatMessages.size(); i++) {
            if(i > 0) json += ",";
            json += "{\"username\":\"" + chatMessages[i].username + "\",";
            json += "\"content\":\"" + chatMessages[i].content + "\",";
            json += "\"timestamp\":\"" + chatMessages[i].timestamp + "\"}";
        }
        json += "]";
        res.set_content(json, "application/json");
    });
    
    svr.Post("/api/send", [](const Request& req, Response& res) {
        string body = req.body;
        
        size_t usernamePos = body.find("\"username\":\"");
        size_t messagePos = body.find("\"message\":\"");
        
        if(usernamePos != string::npos && messagePos != string::npos) {
            usernamePos += 11;
            size_t usernameEnd = body.find("\"", usernamePos);
            messagePos += 10;
            size_t messageEnd = body.find("\"", messagePos);
            
            if(usernameEnd != string::npos && messageEnd != string::npos) {
                string username = body.substr(usernamePos, usernameEnd - usernamePos);
                string content = body.substr(messagePos, messageEnd - messagePos);
                
                lock_guard<mutex> lock(messagesMutex);
                chatMessages.push_back({username, content, getCurrentTime()});
                
                cout << "[" << getCurrentTime() << "] " << username << ": " << content << endl;
                
                res.set_content("OK", "text/plain");
                return;
            }
        }
        res.status = 400;
        res.set_content("Error", "text/plain");
    });
    
    svr.Post("/api/clear", [](const Request& req, Response& res) {
        lock_guard<mutex> lock(messagesMutex);
        chatMessages.clear();
        cout << "[" << getCurrentTime() << "] Чат очищен" << endl;
        res.set_content("OK", "text/plain");
    });
    
    svr.listen("0.0.0.0", 8080);
    return 0;
}
