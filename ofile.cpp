#include <fstream>
#include <iostream>
using namespace std;

int main() {
  ofstream fout("output.txt"); // 打开文件（如果不存在就创建）

  if (!fout.is_open()) { // 判断是否打开成功
    cerr << "无法打开文件！\n";
    return 1;
  }

  fout << "Hello, file I/O!" << endl;
  int a = 42;
  double b = 3.14;
  fout << "a = " << a << ", b = " << b << endl;

  fout.close(); // 关闭文件（也可以依赖析构自动关闭）
  return 0;
}
