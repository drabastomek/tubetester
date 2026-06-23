#ifndef KOREN_H
#define KOREN_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct koren_triode_params {
    double MU;  // amplification factor
    double KG1; // grid current factor
    double KP;  // plate current factor
    double EX;  // exponent
    double KVB; // plate voltage factor
    double VCT; // unused: eq. (4) uses EG only; kept for callers / future extensions
} koren_triode_params;

typedef struct simulation_params_triode {
    double Vp_min;   // minimum plate voltage
    double Vp_max;   // maximum plate voltage
    double Vp_step;  // plate voltage step 
    double Vg_min;   // minimum grid voltage
    double Vg_max;   // maximum grid voltage
    double Vg_step;  // grid voltage step
} simulation_params_triode;


typedef struct koren_pentode_params {
    double MU;   // amplification factor for screen current
    double KG1;  // grid current factor for plate current
    double KG2;  // screen current factor
    double KP;   // plate current factor
    double EX;   // exponent
    double KVB;  // plate voltage factor
    double KVB1; // optional; 0 => f = sqrt(KVB + V_g2^2)
    double KVB2; // atan knee for plate (not the same as KVB)
    double VCT;  // unused: eq. (4) uses EG only; kept for callers / future extensions
} koren_pentode_params;

typedef struct simulation_params_pentode {
    double Vp_min;    // minimum plate voltage
    double Vp_max;    // maximum plate voltage
    double Vp_step;   // plate voltage step
    double Vs_min;    // minimum screen voltage
    double Vs_max;    // maximum screen voltage
    double Vs_step;   // screen voltage step
    double Vg_min;    // minimum grid voltage
    double Vg_max;    // maximum grid voltage
    double Vg_step;   // grid voltage step
} simulation_params_pentode;

double koren_triode(double Vp, double Vg, koren_triode_params p); // triode current
double koren_pentode(double Va, double Vg1, double Vg2, koren_pentode_params p); // pentode current
double koren_pentode_screen(double Vg1, double Vg2, koren_pentode_params p); // pentode screen current

// read parameters from file
koren_triode_params read_params_triode(const char *filename);
koren_pentode_params read_params_pentode(const char *filename);
simulation_params_triode read_sim_params_triode(const char *filename);
simulation_params_pentode read_sim_params_pentode(const char *filename);

// simulation
void simulate_triode(koren_triode_params p, simulation_params_triode s, char *output_file);
void simulate_pentode(koren_pentode_params p, simulation_params_pentode s, char *output_file);
void simulate_pentode_screen(koren_pentode_params p, simulation_params_pentode s, char *output_file);

#endif