#include <iostream>
#include <string>
#include <curl/curl.h>
// Prueba unitaria de conectividad HTTP
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Función auxiliar para buscar el género en el texto JSON sin usar librerías externas
std::string extraer_genero(const std::string& json_txt) {
    std::string clave = "\"gender\":";
    size_t pos = json_txt.find(clave);
    
    if (pos == std::string::npos) {
        return "NO_DEFINIDO";
    }
    
    // Nos movemos al inicio del valor (saltando "gender": y las comillas iniciales)
    pos += clave.length();
    size_t inicio_comilla = json_txt.find("\"", pos);
    if (inicio_comilla == std::string::npos) return "NO_DEFINIDO";
    
    size_t fin_comilla = json_txt.find("\"", inicio_comilla + 1);
    if (fin_comilla == std::string::npos) return "NO_DEFINIDO";
    
    // Recortamos el texto que está entre las comillas (ej: MASCULINO o FEMENINO)
    return json_txt.substr(inicio_comilla + 1, fin_comilla - inicio_comilla - 1);
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    long http_code = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        std::string uuid_prueba = "a6466244-8c40-3c4a-8fae-16eae03c1272";
        std::string url_completa = "http://localhost:8080/cpyd/person/" + uuid_prueba;

        curl_easy_setopt(curl, CURLOPT_URL, url_completa.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        std::cout << "Consultando datos del cliente en la API..." << std::endl;

        res = curl_easy_perform(curl);

        if(res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            if (http_code == 200) {
                // ¡Llamamos a nuestra función extractora!
                std::string genero = extraer_genero(readBuffer);
                
                std::cout << "\n--- RESULTADO DEL PROCESAMIENTO ---" << std::endl;
                std::cout << "Género detectado: " << genero << std::endl;
                std::cout << "-----------------------------------" << std::endl;
            } else {
                std::cerr << "Error: El servidor respondió con código " << http_code << std::endl;
            }
        } else {
            std::cerr << "Error de red cURL: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
