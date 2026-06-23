#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "koren.h"

double koren_triode(double Vp, double Vg, koren_triode_params p) {   
    // https://whitecottage.org.uk/guitar-and-audio/valve-modelling/      
    double EL_Voltage = Vp / p.KP;
    double EL_Exp = exp(
        p.KP * 
        (
            (1 / p.MU) + 
            (Vg + p.VCT) / sqrt(
                p.KVB + pow(Vp, 2)
            )
        )
    );

    double EL_Log = log(1 + EL_Exp);
    double E1 =  EL_Voltage * EL_Log;

    if (E1 <= 0.0) {
        return 0.0;
    }
    double y = pow(E1, p.EX) / p.KG1;
    return y;
}

/* E_pk = ((V_g2/k_p) ln(1 + exp(k_p*(1/μ + (V_g1+V_ct)/f(V_g2)))))^x  — White Cottage pentode section */
static double koren_pentode_epk(double Vg1, double Vg2, koren_pentode_params p) {
    double f_vg2_sq = p.KVB + p.KVB1 * Vg2 + Vg2 * Vg2;
    if (f_vg2_sq <= 0.0) {
        return 0.0;
    }
    double f_vg2 = sqrt(f_vg2_sq);

    double PK_Voltage = Vg2 / p.KP;
    double PK_Exp = exp(
        p.KP *
        (
            (1.0 / p.MU) +
            (Vg1 + p.VCT) / f_vg2
        )
    );

    double PK_Log = log(1.0 + PK_Exp);
    double E_inner = PK_Voltage * PK_Log;

    if (E_inner <= 0.0) {
        return 0.0;
    }
    return pow(E_inner, p.EX);
}

double koren_pentode(double Va, double Vg1, double Vg2, koren_pentode_params p) {
    // https://whitecottage.org.uk/guitar-and-audio/valve-modelling/
    // Koren's TUBE.LIB pentode plate source is (PWR(E1,EX)+PWRS(E1,EX))/KG1*ATAN(...).
    // For E1 > 0, LTspice pwr(E1,EX) and pwrs(E1,EX) both equal E1^EX, so the pair is 2*E1^EX.
    // Text equation (5) shows E1^EX/KG1 only; match the published SPICE netlist here.
    if (p.KVB2 <= 0.0 || p.KG1 <= 0.0) {
        return 0.0;
    }
    double E_pk = koren_pentode_epk(Vg1, Vg2, p);
    if (E_pk <= 0.0) {
        return 0.0;
    }
    return (2.0 * E_pk / p.KG1) * atan(Va / p.KVB2);
}

double koren_pentode_screen(double Vg1, double Vg2, koren_pentode_params p) {
    /*
     * Koren (Tubemodspice): screen current is still eq. (3), not the new E1 form.
     *   IG2 = (EG + EG2/mu)^(3/2) / kG2   for EG + EG2/mu >= 0, else 0
     * See "We continue to use equation (3) for pentode screen grid current" in
     * https://www.normankoren.com/Audio/Tubemodspice_article.html
     * EG  = G1 w.r.t. cathode (here Vg1 + VCT like the plate inner term).
     * EG2 = screen w.r.t. cathode (Vg2).
     */
    double g1_eff;
    double drive;

    if (p.KG2 <= 0.0 || p.MU <= 0.0) {
        return 0.0;
    }
    g1_eff = Vg1 + p.VCT;
    drive = g1_eff + Vg2 / p.MU;
    if (drive <= 0.0) {
        return 0.0;
    }
    return pow(drive, 1.5) / p.KG2;
}

koren_triode_params read_params_triode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }

    char line[512];
    koren_triode_params p = {0};

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: empty or unreadable params file %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: missing data line in %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);

    /* Line 2: MU KG1 KP EX KVB VCT (optional trailing ';' on last field) */
    int n = sscanf(line, "%lf %lf %lf %lf %lf %lf", &p.MU, &p.EX, &p.KG1, &p.KP, &p.KVB, &p.VCT);
    if (n != 6) {
        fprintf(stderr, "Error: expected 6 values on data line in %s (got %d)\n", filename, n);
        exit(1);
    }

    return p;
}

koren_pentode_params read_params_pentode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open pentode params file %s\n", filename);
        exit(1);
    }

    char line[512];
    koren_pentode_params p = {0};

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: empty or unreadable pentode params file %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: missing data line in %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);

    int n = sscanf(
        line,
        "%lf %lf %lf %lf %lf %lf",
        &p.MU,
        &p.EX,
        &p.KG1,
        &p.KG2,
        &p.KP,
        &p.KVB);
    if (n != 6) {
        fprintf(stderr, "Error: expected 6 values on data line in %s (got %d)\n", filename, n);
        exit(1);
    }

    p.KVB1 = 0.0;
    p.VCT = 0.0;
    p.KVB2 = p.KVB;

    return p;
}

simulation_params_triode read_sim_params_triode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open sim params file %s\n", filename);
        exit(1);
    }

    char line[512];
    simulation_params_triode s = {0};

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: empty or unreadable sim params file %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: missing data line in %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);

    /* Line 2: Vp_min Vp_max Vp_step Vg_min Vg_max Vg_step */
    int n = sscanf(
        line,
        "%lf %lf %lf %lf %lf %lf",
        &s.Vp_min,
        &s.Vp_max,
        &s.Vp_step,
        &s.Vg_min,
        &s.Vg_max,
        &s.Vg_step);
    if (n != 6) {
        fprintf(stderr, "Error: expected 6 values on data line in %s (got %d)\n", filename, n);
        exit(1);
    }

    return s;
}

/*
 * Pentode sweep file (e.g. 6L6GC.sim_params):
 *   line 1: Vp_min Vp_max Vp_step Vs_min Vs_max Vs_step Vg_min Vg_max Vg_step
 *   line 2: nine numbers in that order
 */
simulation_params_pentode read_sim_params_pentode(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open pentode sim params file %s\n", filename);
        exit(1);
    }

    char line[512];
    simulation_params_pentode s = {0};

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: empty or unreadable pentode sim params file %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fgets(line, sizeof line, file) == NULL) {
        fprintf(stderr, "Error: missing data line in %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);

    int n = sscanf(
        line,
        "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
        &s.Vp_min,
        &s.Vp_max,
        &s.Vp_step,
        &s.Vs_min,
        &s.Vs_max,
        &s.Vs_step,
        &s.Vg_min,
        &s.Vg_max,
        &s.Vg_step);
    if (n != 9) {
        fprintf(stderr, "Error: expected 9 values on data line in %s (got %d)\n", filename, n);
        exit(1);
    }

    return s;
}

void simulate_triode(koren_triode_params p, simulation_params_triode s, char *output_file) {
    // open the results file 
    FILE *fw = fopen(output_file, "w");
    if (fw == NULL) {
        fprintf(stderr, "Error: could not open output file %s\n", output_file);
        exit(1);
    }

    fprintf(fw, "Vp\tVg\tIp\n");

    for (double Vp = s.Vp_min; Vp <= s.Vp_max; Vp += s.Vp_step) {
        for (double Vg = s.Vg_min; Vg <= s.Vg_max; Vg += s.Vg_step) {
            double Ip = koren_triode(Vp, Vg, p);
            fprintf(fw, "%.2f\t%.2f\t%.9f\n", Vp, Vg, Ip);
        }
    }

    fclose(fw);
}

void simulate_pentode(koren_pentode_params p, simulation_params_pentode s, char *output_file) {
    FILE *fw = fopen(output_file, "w");
    if (fw == NULL) {
        fprintf(stderr, "Error: could not open output file %s\n", output_file);
        exit(1);
    }

    fprintf(fw, "Vp\tVs\tVg\tIa\n");

    for (double Vp = s.Vp_min; Vp <= s.Vp_max; Vp += s.Vp_step) {
        for (double Vs = s.Vs_min; Vs <= s.Vs_max; Vs += s.Vs_step) {
            for (double Vg = s.Vg_min; Vg <= s.Vg_max; Vg += s.Vg_step) {
                double Ia = koren_pentode(Vp, Vg, Vs, p);
                fprintf(fw, "%.2f\t%.2f\t%.2f\t%.9f\n", Vp, Vs, Vg, Ia * 1000.0);
            }
        }
    }

    fclose(fw);
}