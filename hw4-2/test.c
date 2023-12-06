#include <stdio.h>
#include <stdlib.h>

int n;
int neural[50];
int dp[40][40];

int main(int argc, char *argv[]){
    //input
    char input[50];
    scanf("%s", input);
    FILE *input_file = fopen(input, "r");
    if(input_file == NULL){
        printf("could not open file %s\n", input);
        return 1;
    }
    fscanf(input_file, "%d", &n);
    neural[0] = 1;
    for(int i=1; i<=n; i++){
        fscanf(input_file, "%d", &neural[i]);
    }
    neural[n+1] = 0;
    fclose(input_file);

    for(int i = 0;i<n;i++){
        dp[i][i] = 0;
    }

    for (int L = 2; L < n; L++) {
        for (int i = 1; i < n - L + 1; i++) {
            int j = i + L - 1;

            dp[i][j] = 99999;
            for (int k = i; k < j; k++){
                if((dp[i][k] + dp[k + 1][j] + neural[i - 1] * neural[k] * neural[j]) < dp[i][j]){
                    dp[i][j]  = dp[i][k] + dp[k + 1][j] + neural[i - 1] * neural[k] * neural[j];
                }
            }
        }
    }

    //output
    printf("%d", dp[1][n - 1]);
    return 0;
}
