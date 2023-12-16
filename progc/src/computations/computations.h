#ifndef COMPUTATIONS_H
#define COMPUTATIONS_H

struct RouteStream;

// Computation T: the top 10 visited towns.
void computationT(struct RouteStream* stream);

// Computation S: Stats for steps
void computationS(struct RouteStream* stream);

#endif //COMPUTATIONS_H
