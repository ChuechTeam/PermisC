#ifndef COMPUTATIONS_H
#define COMPUTATIONS_H

struct RouteStream;

// Computation D1: the top 10 drivers based on the number of routes taken.
// EXPERIMENTAL! The awk computation works reliably, but this one is much faster.
//               Only available in EXPERIMENTAL_ALGO mode.
void computationD1(struct RouteStream* stream);

// Computation D2: the top 10 drivers based on the distance traveled
// EXPERIMENTAL! The awk computation works reliably, but this one is much faster.
//               Only available in EXPERIMENTAL_ALGO mode.
void computationD2(struct RouteStream* stream);

// Computation L: the top 10 routes with the highest total distance.
// EXPERIMENTAL! Uses a map, and the awk implementation already exists.
//               Only available in EXPERIMENTAL_ALGO mode.
void computationL(struct RouteStream* stream);

// Computation T: the top 10 visited towns.
void computationT(struct RouteStream* stream);

// Computation S: Stats for steps
void computationS(struct RouteStream* stream);

#endif //COMPUTATIONS_H
