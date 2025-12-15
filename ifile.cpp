#include <fstream>
#include <iostream>
#include <string>
using namespace std;

int main() {
  ifstream fin("output.txt");

  if (!fin) { // 等价于 !fin.good()
    cerr << "打开文件失败！\n";
    return 1;
  }

  string word;
  while (fin >> word) { // 以空格、换行等为分隔
    cout << "单词: " << word << endl;
  }
  string line;
  // getline 一直到读完所有行
  while (getline(fin, line)) {
    cout << "读取到一行: " << line << endl;
  }
  fin.close();
  return 0;
}
