#include <bits/stdc++.h>
using namespace std;

int f[100*100000+1];
int val[5000];
 
int main()
{
    int n, s;
    while (scanf("%d%d", &n, &s)==2) {
        memset(f, 0x3f, sizeof(f));
        f[0] = 0;
        for (int i = 0; i<n; i++) {
            int v; scanf("%d", &v);
            val[i] = v;
            for (int j = 1; j<=s; j++)
                f[j*v] = min(f[j*v], j);
        }
        int q; scanf("%d", &q);
        for (int i = 0; i<q; i++) {
            int ans = 101;
            int m; scanf("%d", &m);
            for (int j = 0; j<n; j++)
                for (int k = 0; k<=(s+1)/2; k++) {
                    if (m<val[j]*k) break;
                    ans = min(ans, k+f[m-val[j]*k]);
                }
            if (ans>s) puts("-1");
            else printf("%d\n", ans);
        }
    }
    return 0;
}
