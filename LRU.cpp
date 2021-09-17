#include <iostream>
#include <string>
#include <unordered_map>

#include <clocale>
#include <cpprest/http_client.h>
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Winhttp.lib")

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace utility;

// Estructura LRU
struct  Node {
    std::string key;
    double value;  // se cambia a float debido a que usara en la popularidad
    std::string titulo;  //titulo de la pelicula
    std::string descripcion;  //descripcion de la pelicula
    Node* next;
    Node* prev;

    Node(std::string key = "", double value = -1, std::string tit = "", std::string des = "") {
        this->key = key;
        this->value = value;
        this->titulo = tit;
        this->descripcion = des;
    }
};

struct LinkedList {
    //nodos centinelas
    Node* head;
    Node* tail;
    int count;

    LinkedList() {
        head = new Node();
        tail = new Node();
        head->next = tail;
        tail->prev = head;
        count = 0;
    }

    Node* front() {
        return head->next;
    }
    Node* last() {
        return tail->prev;
    }
    void insert_beetwen(Node* prev, Node* new_node, Node* next) {
        prev->next = new_node;
        next->prev = new_node;
        new_node->next = next;
        new_node->prev = prev;
        ++count;
    }
    void push_front(Node* new_node) {
        insert_beetwen(this->head, new_node, this->head->next);
    }

    Node* remove_last() {
        return remove(last());
    }

    Node* remove(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        --count;
        return node;
    }

};

class LRUCache {
public:
    LRUCache(int size) {
        this->size = size;
    }
    void push(std::string key, double value, std::string tit, std::string des) {
        auto* new_node = new Node(key, value, tit, des);
        if (dicc.size() < size) {
            linked_list.push_front(new_node);
            dicc[key] = new_node; //se espera runtime cost 0
        }
        else {
            linked_list.push_front(new_node);
            auto* last = linked_list.remove_last();
            dicc.erase(last->key);
        }
    }

    std::string get_most_recent_key() {
        if (linked_list.front() != nullptr) {
            return linked_list.front()->key;
        }
        else
            return "";
    }

    Node* get_most_recent_video() {
        if (linked_list.front() != nullptr) {
            return linked_list.front();
        }
        else
            return nullptr;
    }

    int get_value_from_key(std::string key) {
        auto iter = dicc.find(key);
        if (iter == dicc.end())
            return -1;
        auto old_node = dicc[key];
        linked_list.remove_last();
        auto new_node = new Node(old_node->key, old_node->value, old_node->titulo, old_node->descripcion);
        linked_list.push_front(new_node);
        return dicc[key]->value; // se espera costo O(1)
    }

    Node* get_data_from_key(std::string key) {
        auto iter = dicc.find(key);
        if (iter == dicc.end())
            return nullptr;
        auto old_node = dicc[key];
        linked_list.remove_last();
        auto new_node = new Node(old_node->key, old_node->value, old_node->titulo, old_node->descripcion);
        linked_list.push_front(new_node);
        return new_node; // se espera costo O(1)
    }

    void print_content() {
        Node* pFront = linked_list.front();
        while (pFront != nullptr) {
            std::cout << pFront->key << " | " << pFront->value << " | " << pFront->titulo << " | " << pFront->descripcion << std::endl;
            pFront = pFront->next;
        }
    }

private:
    int size;
    std::unordered_map<std::string, Node*> dicc;
    LinkedList linked_list;
};


int main()
{
    int numPaginas = 1;
    std::setlocale(LC_ALL, "");

    for (int pag = 1; pag <= numPaginas; pag++) {
        auto client = http_client(U("https://api.themoviedb.org/3/discover/movie")); 
        client.request(methods::GET,
            uri_builder()
            .append_query(U("sort_by"), "popularity.desc")
            .append_query(U("api_key"), "5384d159278dd45b506fcfee81ad5416")
            .append_query(U("page"), std::to_wstring(pag))
            .to_string()        
            )
            .then([](http_response response) {
                return response.extract_json();
            })
            .then([](json::value json) {
                LRUCache my_cache(3);
                std::string my_key;
                std::string my_titulo;  //titulo de la pelicula
                std::string my_descripcion;  //descripcion de la pelicula

                for (auto& issue : json[L"results"].as_array()) {
                    my_titulo = utility::conversions::to_utf8string(issue[L"title"].as_string());
                    my_descripcion = utility::conversions::to_utf8string(issue[L"overview"].as_string());
                    my_key = issue[L"id"].as_integer();
                    std::cout << issue[L"id"].as_integer() << " | " << issue[L"popularity"].as_double() << " | " << my_titulo << " | " << my_descripcion << std::endl;
                    my_cache.push(my_key, issue[L"popularity"].as_double(), my_titulo, my_descripcion);
                }
                std::cout << "" << std::endl;
                std::cout << "CONTENIDO DE CACHE" << std::endl;
                my_cache.print_content();
            })
            .wait();
    }
    return 0;
}