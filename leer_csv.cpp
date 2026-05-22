#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

// Función para limpiar las comillas molestas de los textos
std::string limpiar_comillas(std::string texto) {
    texto.erase(std::remove(texto.begin(), texto.end(), '"'), texto.end());
    return texto;
}

int main() {
    std::ifstream archivo("reporte_26128.csv");

    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo CSV." << std::endl;
        return 1;
    }

    std::string linea;
    
    // Leemos y saltamos la primera línea de títulos
    if (std::getline(archivo, linea)) {
        std::cout << "Saltando cabecera: " << linea << std::endl;
    }

    std::cout << "\nProcesando las primeras filas corregidas:\n" << std::endl;

    int contador_filas = 0;

    while (std::getline(archivo, linea)) {
        std::stringstream ss(linea);
        std::string celda;
        
        std::string monto_str = "";
        std::string uuid_cliente = "";
        
        int columna_actual = 1;

        // Separamos por punto y coma
        while (std::getline(ss, celda, ';')) {
            celda = limpiar_comillas(celda);
            
            // MONTO APLICADO está en la columna 7
            if (columna_actual == 7) {
                monto_str = celda;
            }
            // ¡CORREGIDO! El UUID real está en la columna 10
            if (columna_actual == 10) {
                uuid_cliente = celda;
            }
            columna_actual++;
        }

        // Si tenemos ambos datos listos, los imprimimos
        if (!monto_str.empty() && !uuid_cliente.empty()) {
            std::cout << "Fila " << contador_filas + 1 
                      << " -> Cliente UUID: " << uuid_cliente 
                      << " | Monto: $" << monto_str << std::endl;
            
            contador_filas++;
        }

        // Dejamos el límite en 10 para la prueba en pantalla
        if (contador_filas >= 10) {
            break;
        }
    }

    archivo.close();
    std::cout << "\n¡Parser de CSV corregido e implementado con éxito!" << std::endl;
    return 0;
}
