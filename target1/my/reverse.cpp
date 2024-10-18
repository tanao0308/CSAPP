#include<iostream>
#include<vector>
#include <algorithm>
using namespace std;
int main() {
    string s;
    vector<string>all;
    while(cin>>s) {
        all.push_back(s);
    }
    reverse(all.begin(), all.end());
    for(auto x:all) cout<<x<<" ";
}