#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <queue>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map> 
#include <map>
#include <vector>
#include <sstream>
using namespace std;

struct Node {
  char data;
  int freq;
  Node *left;
  Node *right;
  int position;

  Node(char data, int freq, Node *left = nullptr, Node *right = nullptr,int position = -1) {
    this->data = data;
    this->freq = freq;
    this->left = left;
    this->right = right;
    this->position = position;
  }
};

struct Compare {
  bool operator()(Node *left, Node *right) {
    if (left->freq == right->freq) {
      if (left->data == right->data)
        return left->position < right->position;
      return left->data > right->data;
    }
    return left->freq > right->freq;
  }
};

void printCodes(Node *root, string str) {
  if (!root)
    return;
  if (root->data != '\0') {
    cout << "Symbol: " << root->data << ", ";
    cout << "Frequency: " << root->freq;
    cout << ", Code: " << str << endl;
  }
  printCodes(root->left, str + "0");
  printCodes(root->right, str + "1");
}

Node *HuffmanCodes(vector<char> data, vector<int> freq, int size) {
  priority_queue<Node *, vector<Node *>, Compare> pq;
  for (int i = 0; i < size; i++) {
    pq.push(new Node(data[i], freq[i]));
  }
  int position = 0;
  while (pq.size() > 1) {
    Node *left = pq.top();
    pq.pop();
    Node *right = pq.top();
    pq.pop();

    int sum = left->freq + right->freq;
    pq.push(new Node('\0', sum, left, right, position));
    position++;
  }
  Node *root = pq.top();
  printCodes(root, "");
  return root;
}

char decode(Node* root, string code) {
  Node* curr = root;
  for (int i = 0; i < code.size(); i++) {
    if (code[i] == '0')
      curr = curr->left;
    else
      curr = curr->right;
    if (!curr->left && !curr->right)
      return curr->data;
  }
  return '\0';
}

void fireman(int) {
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    break;
  }
}

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  signal(SIGCHLD, fireman);
  if (argc < 2) {
    cerr << "ERROR, no port provided" << endl;
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    cerr << "ERROR opening socket" << endl;
    exit(1);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    cerr << "ERROR on binding" << endl;
    exit(1);
  }
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  int port = atoi(argv[1]);

  // reading character and frequency information from input file and storing in vectors
  string line;
  vector<char> data;
  vector<int> freq;
  while (getline(cin, line)) {
    data.push_back(line[0]);
    string number;
    for (int i = 2; i < line.size(); i++)
      number += line[i];
    freq.push_back(stoi(number));
  }
  
  int numOfChars = data.size();
  Node *root = HuffmanCodes(data, freq, numOfChars);

  // accept incoming connections
  while (true) {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    if (fork() == 0) {
      if (newsockfd < 0) {
        cerr << "ERROR on accept" << endl;
        exit(1);
      }
      for(int i = 0; i<numOfChars; i++){
        int size;
        n = read(newsockfd, &size, sizeof(int));
        if (n < 0) {
          cerr << "ERROR reading from socket" << endl;
          exit(1);
        }
        // receiving the binary code from the client
        char *binaryString = new char[size + 1];
        bzero(binaryString, size + 1);
        n = read(newsockfd, binaryString, size);
        if (n < 0) {
          cerr << "ERROR reading from socket" << endl;
          exit(1);
        }
        //cout << "Message from client: " << binaryString << endl;
        
        // using the generated Huffman tree to decode the binary code
        string decodedChar;
        Node *curr = root;
        for (int i = 0; i < size; i++) {
          if (binaryString[i] == '0')
            curr = curr->left;
          else
            curr = curr->right;
          if (curr->left == nullptr && curr->right == nullptr) {
            decodedChar = curr->data;
            curr = root;
          }
        }

        // sending the char to client to be used for contructing the final message 
        n = write(newsockfd, decodedChar.c_str(), decodedChar.size());
        if (n < 0) {
          cerr << "ERROR writing to socket" << endl;
          exit(1);
        }
        delete[] binaryString;
      }
      close(newsockfd);
      _exit(0);
    }
  }
  close(sockfd);
  return 0;
}
