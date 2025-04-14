struct Coord_rpc {
    int x;
    int y;
};

struct set_value_peticion {
    int key;
    string value1<>;
    int N_value2;
    double V_value2 [32];
    struct Coord_rpc value3;
};

struct get_value_respuesta {
    int status;
    string value1<>;
    int N_value2; 
    double V_value2 [32];
    struct Coord_rpc value3;
};

program CLAVES  {
    version CLAVESVER {
        int destroy_rpc(void) = 1;
        int set_value_rpc(set_value_peticion p) = 2;
        struct get_value_respuesta get_value_rpc(int key) = 3;
        int modify_value_rpc(set_value_peticion p) = 4;
        int exist_rpc(int key) = 5;
        int delete_rpc(int key) = 6;
    } = 1
    ;
} = 31415926;
