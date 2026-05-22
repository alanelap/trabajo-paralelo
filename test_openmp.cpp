#include <iostream>
#include <omp.h>

int main() {
    // Le indicamos a OpenMP que use 4 hilos de prueba
    omp_set_num_threads(4);

    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        std::cout << "Hola desde el hilo " << id << " corriendo en Ubuntu!" << std::endl;
    }
    return 0;
}

