# Procesamiento Paralelo de Transacciones - Computación Paralela y Distribuida 2026

Este repositorio contiene la solución desarrollada para la asignatura **Computación Paralela y Distribuida** (Universidad Tecnológica Metropolitana). El proyecto implementa un sistema de alto rendimiento en C++ que descarga, procesa y analiza flujos masivos de transacciones financieras utilizando paralelismo de memoria compartida con **OpenMP** y consumo de servicios **API REST**.

---

## 📋 Descripción del Problema (Caso Cruz Morada)

La cadena de farmacias "Cruz Morada" genera diariamente archivos consolidados con miles de transacciones de ventas distribuidos en servidores SFTP remotos. El objetivo fundamental del sistema es calcular el promedio de los montos aplicados en las transacciones, segmentando los resultados estrictamente por el género del cliente (`MASCULINO` / `FEMENINO`).

### El Desafío Técnico
1. **Volumen de Datos:** Procesamiento secuencial ineficiente ante archivos de más de 21,000 registros de transacciones.
2. **Latencia de Red:** El género no se encuentra en el reporte original; debe consultarse dinámicamente mediante peticiones HTTP a un servicio API REST externo por cada cliente (UUID), lo que introduce un cuello de botella crítico.
3. **Concurrencia:** Necesidad de paralelizar el ciclo de lectura y las consultas de red evitando condiciones de carrera (*race conditions*) y corrupción de memoria en las estructuras compartidas.

---

## 🛠️ Arquitectura del Sistema y Solución

El proyecto evolucionó a través de fases incrementales para asegurar la estabilidad, el manejo correcto de errores y la escalabilidad del software:

* **Descargador SFTP (`descargar_sftp.cpp`):** Automatiza la conexión segura al host remoto mediante la biblioteca `libcurl` (con soporte SSH/SFTP) y descarga el archivo de datos objetivo (`reporte_26128.csv`).
* **Versión Secuencial (`tarea_secuencial.cpp`):** Establece la lógica base del parser de datos CSV utilizando punto y coma (`;`) como delimitador, limpieza de comillas dobles (`"`) y mapeo de columnas (Columna 7 para Monto, Columna 10 para UUID).
* **Simulador de API REST (`servidor_falso.py`):** Script auxiliar en Python que emula el comportamiento del endpoint remoto `/cpyd/person/{uuid}` devolviendo respuestas JSON estructuradas (`200 OK`) para sortear caídas de servicio de la infraestructura principal.
* **Versión Paralela Multihilo (`tarea_paralela.cpp`):** El núcleo optimizado del sistema. Carga los registros en un vector dinámico y distribuye la carga de procesamiento HTTP y operaciones aritméticas de forma concurrente mediante directivas avanzadas de OpenMP.

---

## 🚀 Optimización Concurrente con OpenMP

Para cumplir con las restricciones del problema y maximizar el rendimiento en arquitecturas modernas, el código paralelo implementa tres estrategias clave:

### 1. Planificación Dinámica
Se utiliza `#pragma omp parallel for schedule(dynamic)` para la distribución de filas. Dado que el tiempo de respuesta de las peticiones HTTP externas es variable, la planificación dinámica evita el desequilibrio de carga (*load imbalance*) asignando tareas a los hilos a medida que se liberan.

### 2. Control de Secciones Críticas (Thread-Safety)
Para evitar fallos de segmentación (*segmentation faults*) y corrupción de datos al usar estructuras globales compartidas, se implementaron mecanismos de exclusión mutua:
* **Caché en Memoria Compartida (`std::unordered_map`):** Protegida mediante `#pragma omp critical(cache_access)`. Solo un hilo a la vez puede verificar o registrar nuevos clientes consultados, mitigando el impacto de latencia de red en UUIDs repetidos.
* **Manejo de Errores Asíncrono:** La escritura en el archivo persistente `log.txt` (errores de red o respuestas HTTP distintas de 200) está blindada bajo una sección crítica dedicada para impedir colisiones de E/S.

### 3. Operaciones Atómicas
Las variables globales encargadas de almacenar las sumas acumuladas y los contadores de transacciones se actualizan concurrentemente utilizando `#pragma omp atomic`, minimizando el bloqueo del procesador en comparación con una sección crítica estándar.

---

## 📊 Análisis de Rendimiento y Escalabilidad

Las pruebas empíricas se ejecutaron en un entorno virtualizado **Ubuntu 24.04 LTS** montado sobre un procesador **Apple Silicon M2** empleando el hipervisor Parallels Desktop, configurado estrictamente con **4 núcleos de CPU virtuales**.

### Tabla de Métricas Obtenidas

| Cantidad de Hilos (OpenMP) | Tiempo de Ejecución | Speedup Relativo | Estado de Logs |
| :--- | :--- | :--- | :--- |
| **1 Hilo (Secuencial)** | 2.53983 segundos | 1.00 (Base) | Limpio, sin errores |
| **2 Hilos** | 1.92401 segundos | 1.32x | Limpio, sin errores |
| **4 Hilos (Óptimo)** | 1.68281 segundos | 1.50x | Limpio, sin errores |
| **6 Hilos** | 1.76795 segundos | 1.43x | Limpio, sin errores |

### Conclusiones Técnicas del Comportamiento del Hardware
* **Punto de Equilibrio Óptimo (4 Hilos):** Se observa un escalamiento eficiente y la tasa más alta de procesamiento al existir una relación simétrica **1:1** entre los hilos asignados por software y las vCPUs físicas disponibles en la máquina virtual.
* **Fenómeno de Sobre-suscripción (6 Hilos):** Al forzar una cantidad de hilos superior a los núcleos físicos asignados al sistema operativo (6 hilos frente a 4 CPUs), el rendimiento decrece debido a la sobrecarga por **Cambio de Contexto (*Context Switching*)**. Los hilos se ven obligados a competir y alternar artificialmente el uso del hardware, sumando latencia innecesaria al tiempo de cómputo global. Esto valida empíricamente los postulados de la **Ley de Amdahl**.

---

## 🛠️ Instrucciones de Compilación y Ejecución

El proyecto cuenta con un archivo `Makefile` que automatiza por completo las banderas de optimización y el enlazado de bibliotecas dinámicas.

### Prerrequisitos
Asegúrese de contar con las siguientes herramientas instaladas en su distribución Linux:
```bash
sudo apt update
sudo apt install -y build-essential libcurl4-openssl-dev libssh2-1-dev
