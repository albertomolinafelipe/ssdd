struct log_entry {
    string username<>;
    string operation<>;
    string datetime<>;
};

program LOGGER_PROG {
    version LOGGER_VERS {
        int log_operation(log_entry) = 1;
    } = 1;
} = 0x31415926;
