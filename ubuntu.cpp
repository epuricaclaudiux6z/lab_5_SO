#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

int main() {
    const int num_processes = 10;
    const int chunk_size = 1000;
    std::vector<int> c2p_read_ends(num_processes);
    std::vector<pid_t> pids(num_processes);

    for (int i = 0; i < num_processes; ++i) {
        int p2c[2];
        int c2p[2];

        if (pipe(p2c) == -1 || pipe(c2p) == -1) return 1;

        pid_t pid = fork();
        if (pid < 0) return 1;

        if (pid == 0) {
            close(p2c[1]);
            close(c2p[0]);

            int start, end;
            read(p2c[0], &start, sizeof(start));
            read(p2c[0], &end, sizeof(end));
            close(p2c[0]);

            for (int num = start; num <= end; ++num) {
                if (is_prime(num)) {
                    write(c2p[1], &num, sizeof(num));
                }
            }
            close(c2p[1]);
            exit(0);
        } else {
            close(p2c[0]);
            close(c2p[1]);

            int start = i * chunk_size + 1;
            int end = (i + 1) * chunk_size;

            write(p2c[1], &start, sizeof(start));
            write(p2c[1], &end, sizeof(end));
            close(p2c[1]);

            c2p_read_ends[i] = c2p[0];
            pids[i] = pid;
        }
    }

    for (int i = 0; i < num_processes; ++i) {
        int prime;
        while (read(c2p_read_ends[i], &prime, sizeof(prime)) > 0) {
            std::cout << prime << " ";
        }
        close(c2p_read_ends[i]);
        waitpid(pids[i], nullptr, 0);
    }
    std::cout << "\n";
    return 0;
}
