#include <bits/stdc++.h>
using namespace std;
 
char name[1000001][30];
char course[1000001][30];
 
int main()
{
    int n, m;
    while (scanf("%d%d",&n,&m)==2) {
        for (int i = 0; i<=1000000; i++) {
            name[i][0] = 0;
            course[i][0] = 0;
        }
        scanf("%*s%*s");
        for (int i = 0; i<n; i++) {
            char buf[30];
            int x;
            scanf("%s%d", buf, &x);
            strcpy(name[x], buf);
        }
        scanf("%*s%*s%*s");
        for (int i = 0; i<m; i++) {
            char buf[30];
            int x;
            scanf("%*s%d%s", &x, buf);
            strcpy(course[x], buf);
        }
        puts("Name StuNum CourseName");
        for (int i = 0; i<1000000; i++)
            if (name[i][0]) {
                printf("%s %d ", name[i], i);
                if (course[i][0])
                    puts(course[i]);
                else
                    puts("NULL");
            }
    }
    return 0;
}
 
