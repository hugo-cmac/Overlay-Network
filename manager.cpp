#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <iterator>

using namespace std;

enum DIRECCION {
    UP = 0,
    RIGHT,
    DOWN,
    LEFT
};

typedef struct Coordinate{
    uint16_t x;
    uint16_t y;

    Coordinate(uint16_t x, uint16_t y): x(x), y(y){}
    
    bool operator<(const Coordinate &ob) const{
		return x < ob.x || (x == ob.x && y < ob.y);
	}
    struct Coordinate left(){
        return Coordinate(x-1, y);
    }
    struct Coordinate right(){
        return Coordinate(x+1, y);
    }
    struct Coordinate up(){
        return Coordinate(x, y-1);
    }
    struct Coordinate down(){
        return Coordinate(x, y+1);
    }
    
}coordinates;


class Client{
    private:
        uint16_t certificate;
        uint16_t id;
        uint8_t type; //2-ligacoes/3-ligacoes/4-ligacoes
        int tunnel = 0;
        coordinates coord = Coordinate(0,0);
    public :
        Client(uint16_t certificate, uint16_t id, uint16_t type, int tunnel, coordinates coord){
            this->certificate = certificate;
            this->id = id;
            this->type = type;
            this->coord = coord;
        }

        coordinates getCOords(){
            return coord;
        }

};

class Matrix{
    private:
        uint16_t side ;
        uint16_t count ;

        coordinates coord = Coordinate(0,0);

        map<uint16_t, Client*> ClientList;
        map<coordinates, uint16_t> matrix;

        stack<uint8_t> neighbors;
    public:
        Matrix(){
            this->side = 1;
            this->count = 0; 
        }

        void newClient(uint16_t id, int socket, uint32_t ip){
            count++;

            Client *n = new Client(0, id, getTypeOfNode(), socket, coord);
            ClientList[id] = n;
            matrix[coord] = id;

            //selectNeighbor();
            
            
            nextCoord();
            selectNeighbors();
        }

        void advertiseNeighbors(){
            uint8_t dir;
            while (!neighbors.empty()){
                
            }
            
            
        }

        uint16_t getNeighbor(uint16_t dir){
            uint16_t id = 0;
            switch (dir) {
                case UP:
                    id = matrix[coord.up()];
                    break;
                case RIGHT:
                    id = matrix[coord.right()];
                    break;
                case DOWN:
                    id = matrix[coord.down()];
                    break;
                case LEFT:
                    id = matrix[coord.left()];
                    break;
            }
            return id;
        }

        void selectNeighbors(){
            if (coord.x) 
                neighbors.push(LEFT);
            
            if (coord.y) 
                neighbors.push(UP);
        }

        void nextCoord(){
            if (count == side*side){
                side ++;
                coord.x = side - 1;
                coord.y = 0;
            } else {
                if (coord.x == side-1){
                    if (coord.y == side-2){
                        coord.x = 0;
                    }
                    coord.y++;
                } else {
                    coord.x++;
                }
            }
        }

        int getTypeOfNode(){
            if (coord.x == 0 && coord.y == 0){
                return 2;
            } else if (coord.x == 0 || coord.y == 0) {
                return 3;
            } else {
                return 4;
            }
        }

        void printMatrix(){
            coordinates c = Coordinate(0,0);

            for (size_t y = 0; y < side; y++){
                for (size_t x = 0; x < side; x++){
                    c.x = x;
                    c.y = y;
                    cout << "Node: " + to_string(matrix[c]) + "   ";
                }
                cout << "\n";
            }
        }
};


int main(){
    Matrix* matrix = new Matrix();

    // for (size_t i = 0; i < 20; i++){
    //     matrix->newClient(i+1, 0, 0);
    //     getchar();
    //     matrix->printMatrix();
    // }

    if (-1){
        cout<<"um\n";
    }
    
    

    return 0;
}