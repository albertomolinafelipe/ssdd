# FINAL SSDD

- `server.c`: Servidor principal
- `logger_server.c`: Servidor RPC de log
- `client.py`: Cliente que se comunica con el servidor y otros clientes
- `date.py`: Servidor HTTP que devuelve la fecha

## Compilación

Para compilar los componentes en C, basta con ejecutar:
```bash
make
```

Los binarios generados se guardan en `bin/`.

## Ejecución

- Servidor principal:

```bash
  env LOG_RPC_IP=<ip_logger_server> ./bin/server -p 6677
```

- Servidor RPC:

```bash
  ./bin/logger_server
```

- Cliente (modo interactivo):

```bash
  python3 client.py -s <ip_server> -p 6677
```

- Servidor web (fecha):

```bash
  python3 date.py
```

## Uso del script `debug.sh`

El script `debug.sh` facilita las pruebas dentro del entorno `u20-docker`. Debe ejecutarse desde un contenedor con el proyecto copiado dentro.

Modos disponibles:

- `-s`: Compila y lanza el servidor principal (pensado para el contenedor 1)
- `-r`: Compila y lanza el servidor RPC (pensado para el contenedor 2)
- `-c <fichero>`: Lanza el servidor web y ejecuta el cliente leyendo comandos del fichero (en los demás contenedores)

El script usa las IPs definidas en `../machines` y el puerto 6677 por defecto.
