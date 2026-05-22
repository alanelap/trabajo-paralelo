// Control de versiones: Fase 3 y Fase 4 finalizadas con éxito de forma secuencial y paralela.



#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <curl/curl.h>
#include <omp.h> // <--- Librería oficial de OpenMP

std::unordered_map<std::string, std::string> cache_clientes;

double suma_femenino = 0.0;
long cuenta_femenino = 0;

double suma_masculino = 0.0;
long cuenta_masculino = 0;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string limpiar_comillas(std::string texto) {
    texto.erase(std::remove(texto.begin(), texto.end(), '"'), texto.end());
    return texto;
}

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

std::string consultar_genero_api(const std::string& uuid) {
    std::string genero_detectado = "NO_DEFINIDO";
    bool encontrado_en_cache = false;

    // 1. PASO CRÍTICO: Revisamos la caché de forma segura
    #pragma omp critical(cache_access)
    {
        if (cache_clientes.find(uuid) != cache_clientes.end()) {
            genero_detectado = cache_clientes[uuid];
            encontrado_en_cache = true; // Marcamos que ya lo encontramos
        }
    }

    // Si ya lo encontramos en la caché, saltamos directo al final de la función
    if (encontrado_en_cache) {
        return genero_detectado; // <--- Este return aquí AFUERA del bloque critical es 100% seguro
    }

    // 2. PASO DE RED: Si no estaba en caché, hacemos la petición HTTP (fuera de la sección crítica para no ralentizar a los otros hilos)
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    long http_code = 0;

    if(curl) {
        std::string url = "http://localhost:8080/cpyd/person/" + uuid;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        CURLcode res = curl_easy_perform(curl);
        if(res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                genero_detectado = extraer_genero(readBuffer);
            } else {
                // Manejo de errores escribiendo en log.txt de forma segura
                #pragma omp critical(log_file_write)
                {
                    std::ofstream log_file("log.txt", std::ios::app);
                    if (log_file.is_open()) {
                        log_file << "Error al consultar cliente " << uuid << ": Codigo HTTP " << http_code << std::endl;
                        log_file.close();
                    }
                }
            }
        } else {
            // Manejo de errores de red en log.txt
            #pragma omp critical(log_file_write)
            {
                std::ofstream log_file("log.txt", std::ios::app);
                if (log_file.is_open()) {
                    log_file << "Error de red cURL para cliente " << uuid << ": " << curl_easy_strerror(res) << std::endl;
                    log_file.close();
                }
            }
        }
        curl_easy_cleanup(curl);
    }

    // 3. PASO DE ESCRITURA: Guardamos el nuevo resultado en la caché compartida
    if (genero_detectado != "NO_DEFINIDO") {
        #pragma omp critical(cache_access)
        {
            cache_clientes[uuid] = genero_detectado;
        }
    }

    // Único punto de salida para solicitudes nuevas
    return genero_detectado;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::ifstream archivo("reporte_26128.csv");
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo CSV." << std::endl;
        return 1;
    }

    std::string linea;
    std::getline(archivo, linea); // Saltamos cabecera

    // Guardamos todas las líneas del archivo en memoria (un vector) para que OpenMP pueda repartirlas fácilmente
    std::vector<std::string> lineas;
    while (std::getline(archivo, linea)) {
        lineas.push_back(linea);
    }
    archivo.close();

    long total_lineas = lineas.size();
    std::cout << "Líneas cargadas en memoria: " << total_lineas << ". Iniciando procesamiento paralelo..." << std::endl;

    // Medimos el tiempo inicial usando el reloj de OpenMP
    double tiempo_inicio = omp_get_wtime();

    // OPENMP MÁGICO: Repartimos el procesamiento de las filas entre múltiples hilos de tu M2
    #pragma omp parallel for schedule(dynamic)
    for (long i = 0; i < total_lineas; i++) {
        std::stringstream ss(lineas[i]);
        std::string celda;
        std::string monto_str = "";
        std::string uuid_cliente = "";
        int columna_actual = 1;

        while (std::getline(ss, celda, ';')) {
            celda = limpiar_comillas(celda);
            if (columna_actual == 7)  monto_str = celda;
            if (columna_actual == 10) uuid_cliente = celda;
            columna_actual++;
        }

        if (!monto_str.empty() && !uuid_cliente.empty()) {
            double monto = std::stod(monto_str);
            std::string genero = consultar_genero_api(uuid_cliente);

            // OPENMP PROTECCIÓN: Evitamos condiciones de carrera en los acumuladores globales
            if (genero == "FEMENINO") {
                #pragma omp atomic
                suma_femenino += monto;
                #pragma omp atomic
                cuenta_femenino++;
            } else if (genero == "MASCULINO") {
                #pragma omp atomic
                suma_masculino += monto;
                #pragma omp atomic
                cuenta_masculino++;
            }
        }
    }

// Medimos el tiempo final
    double tiempo_total = omp_get_wtime() - tiempo_inicio;

    curl_global_cleanup();

    // 1. ESCRIBIR EN EL ARCHIVO FISICO (resultados.txt)
    std::ofstream archivo_salida("resultados.txt");
    if (archivo_salida.is_open()) {
        if (cuenta_femenino > 0) {
            archivo_salida << "FEMENINO = " << (suma_femenino / cuenta_femenino) << std::endl;
        }
        if (cuenta_masculino > 0) {
            archivo_salida << "MASCULINO = " << (suma_masculino / cuenta_masculino) << std::endl;
        }
        archivo_salida << "TIEMPO = " << tiempo_total << " segundos" << std::endl;
        archivo_salida.close();
        std::cout << "\n[!] Archivo 'resultados.txt' generado con exito." << std::endl;
    } else {
        std::cerr << "Error: No se pudo crear el archivo resultados.txt" << std::endl;
    }

    // 2. MANTENER LA IMPRESIÓN EN TERMINAL (Para que sigas viendo tus datos al correrlo)
    std::cout << "\n================ RESULTADOS PARALELOS ================" << std::endl;
    if (cuenta_femenino > 0) std::cout << "FEMENINO = " << (suma_femenino / cuenta_femenino) << std::endl;
    if (cuenta_masculino > 0) std::cout << "MASCULINO = " << (suma_masculino / cuenta_masculino) << std::endl;
    std::cout << "TIEMPO = " << tiempo_total << " segundos" << std::endl;
    std::cout << "======================================================" << std::endl;

    return 0;
}
