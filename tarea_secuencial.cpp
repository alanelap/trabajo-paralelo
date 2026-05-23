#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <curl/curl.h>

// Estructura para almacenar la caché de hilos
std::unordered_map<std::string, std::string> cache_clientes;

// Variables para acumular las compras por género [cite: 71]
double suma_femenino = 0.0;
long cuenta_femenino = 0;

double suma_masculino = 0.0;
long cuenta_masculino = 0;

// Función cURL para capturar el texto de la API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Función para remover comillas
std::string limpiar_comillas(std::string texto) {
    texto.erase(std::remove(texto.begin(), texto.end(), '"'), texto.end());
    return texto;
}

// Función que extrae el género del JSON sin librerías externas
std::string extraer_genero(const std::string& json_txt) {
    std::string clave = "\"gender\":";
    size_t pos = json_txt.find(clave);
    if (pos == std::string::npos) return "NO_DEFINIDO";
    
    pos += clave.length();
    size_t inicio_comilla = json_txt.find("\"", pos);
    if (inicio_comilla == std::string::npos) return "NO_DEFINIDO";
    
    size_t fin_comilla = json_txt.find("\"", inicio_comilla + 1);
    if (fin_comilla == std::string::npos) return "NO_DEFINIDO";
    
    return json_txt.substr(inicio_comilla + 1, fin_comilla - inicio_comilla - 1);
}

// Módulo modular para consultar la API 
std::string consultar_genero_api(const std::string& uuid) {
    // 1. Primero revisamos si ya lo conocemos en la caché 
    if (cache_clientes.find(uuid) != cache_clientes.end()) {
        return cache_clientes[uuid];
    }

    // 2. Si no está en caché, le preguntamos a la API [cite: 65]
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    long http_code = 0;
    std::string genero_detectado = "NO_DEFINIDO";

    if(curl) {
        // Apuntamos a tu servidor simulado local en puerto 8080
        std::string url = "http://localhost:8080/cpyd/person/" + uuid;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        CURLcode res = curl_easy_perform(curl);
        if(res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                genero_detectado = extraer_genero(readBuffer);
            }
        }
        curl_easy_cleanup(curl);
    }

    // 3. Lo guardamos en la caché antes de retornar para la próxima vez 
    cache_clientes[uuid] = genero_detectado;
    return genero_detectado;
}

int main() {
    // Inicialización global de curl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::ifstream archivo("reporte_26128.csv");
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo CSV." << std::endl;
        return 1;
    }

    std::string linea;
    // Saltamos la primera línea (cabecera)
    std::getline(archivo, linea);

    std::cout << "Procesando registros de transacciones de forma secuencial..." << std::endl;
    int procesados = 0;

    // Procesamos el CSV línea por línea [cite: 62]
    while (std::getline(archivo, linea)) {
        std::stringstream ss(linea);
        std::string celda;
        std::string monto_str = "";
        std::string uuid_cliente = "";
        int columna_actual = 1;

        while (std::getline(ss, celda, ';')) {
            celda = limpiar_comillas(celda);
            if (columna_actual == 7)  monto_str = celda;   // Monto
            if (columna_actual == 10) uuid_cliente = celda; // UUID
            columna_actual++;
        }

        if (!monto_str.empty() && !uuid_cliente.empty()) {
            double monto = std::stod(monto_str);
            
            // Consultamos el género (utilizará caché de manera inteligente) [cite: 65, 66]
            std::string genero = consultar_genero_api(uuid_cliente);

            // Asociamos el monto al género y acumulamos [cite: 75]
            if (genero == "FEMENINO") {
                suma_femenino += monto;
                cuenta_femenino++;
            } else if (genero == "MASCULINO") {
                suma_masculino += monto;
                cuenta_masculino++;
            }
            
            procesados++;
            // Mostramos feedback rápido en la terminal cada 50 registros para ver avance
            if (procesados % 50 == 0) {
                std::cout << "Registros analizados de forma secuencial: " << procesados << "..." << std::endl;
            }
       }
    }

    archivo.close();
    curl_global_cleanup();

    // Calculamos y mostramos los promedios finales solicitados en el formato correcto
    std::cout << "\n================ RESULTADOS ================" << std::endl;
    if (cuenta_femenino > 0) {
        std::cout << "FEMENINO = " << (suma_femenino / cuenta_femenino) << std::endl;
    }
    if (cuenta_masculino > 0) {
        std::cout << "MASCULINO = " << (suma_masculino / cuenta_masculino) << std::endl;
    }
    std::cout << "============================================" << std::endl;

    return 0;
}

