#include "Pole.h"

void initPole(struct Pole *pole, int farba) {
    pole->farba = farba;
}

int getFarba(const struct Pole *pole) {
    return pole->farba;
}

void zmenFarba(struct Pole *pole) {
    pole->farba = (pole->farba + 1) % 2;
}
