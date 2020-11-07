//
// Created by alex on 31/10/20.
//

#ifndef TENINROW_ARBITRO_H
#define TENINROW_ARBITRO_H

#include "Cliente.h"
#include "Jogo.h"
#include "Administrador.h"
#include "JogosCliente.h"

typedef struct Arbitro {
    Cliente *cliente;
    Jogo *jogos;
    Administrador *administrador;
    JogosCliente *jogosCliente;
} Arbitro;


#endif //TENINROW_ARBITRO_H
