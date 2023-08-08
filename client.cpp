#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
using namespace std;

struct ThreadData {
  int sockfd;
  string key;
  vector<int> values;
  char *finalMessage;
  pthread_mutex_t *mutex;
};

// thread function //
void *threadFunction(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  string str = data->key;
  char message[str.size() + 1];
  strcpy(message, str.c_str());
  int sMessage = strlen(message);
  pthread_mutex_lock(data->mutex);
  int n = write(data->sockfd, &sMessage, sizeof(int));
  n = write(data->sockfd, message, sMessage);
  pthread_mutex_unlock(data->mutex);

  vector<int> positions = data->values;
  char receivedChar;
  n = read(data->sockfd, &receivedChar, sizeof(char));
  //cout << "Message from server: " << receivedChar << endl;
  for (int i = 0; i < positions.size(); i++) {
    int position = positions[i];
    pthread_mutex_lock(data->mutex);
    data->finalMessage[position] = receivedChar;
    pthread_mutex_unlock(data->mutex);
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  if (argc < 3){
    cerr << "usage " << argv[0] << "hostname port" << endl;
    exit(0);
  }
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    cerr << "ERROR opening socket";
  
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    cerr << "ERROR, no such host" << endl;
    exit(0);
  }
  
  // thowing the contents of the compressed file into a map
  int count = 0;
  unordered_map<string, vector<int>> map;
  string key;
  while (cin >> key) {
    vector<int> values;
    int value;
    while (cin >> value) {
      values.push_back(value);
      count++;
      if (cin.peek() == '\n')
        break;
    }
    map[key] = values;
  }
  
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    cerr << "ERROR connecting" << endl;
    exit(1);
  }

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  int numThreads = map.size();
  pthread_t threads[numThreads];
  char finalMessage[256];
  memset(finalMessage, '\0', sizeof(finalMessage));
  ThreadData threadData[numThreads];
  int i = 0;
  for (auto it = map.begin(); it != map.end(); ++it) {
    threadData[i].key = it->first;
    threadData[i].values = it->second;
    threadData[i].sockfd = sockfd;
    threadData[i].finalMessage = finalMessage;
    threadData[i].mutex = &mutex;
    pthread_create(&threads[i], NULL, threadFunction, &threadData[i]);
    pthread_join(threads[i], NULL);
    i++;
  }
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  // print the final message
  cout << "Original message: " << finalMessage << endl;

  close(sockfd);
  return 0;
}