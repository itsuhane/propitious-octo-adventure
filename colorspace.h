#pragma once

#include <cmath>

namespace colorspace {
    class XYZ {
    public:
        double& operator[] (size_t i) { return m_data[i]; }
        const double& operator[] (size_t i) const { return m_data[i]; }

        XYZ(double x, double y, double z) {
            m_data[0] = x;
            m_data[1] = y;
            m_data[2] = z;
        }

        XYZ(const sRGB &srgb) {
            double r_linear = sRGB::gamma_itransform(srgb[0]);
            double g_linear = sRGB::gamma_itransform(srgb[1]);
            double b_linear = sRGB::gamma_itransform(srgb[2]);

            m_data[0] = 0.4124*r_linear + 0.3576*g_linear + 0.1805*b_linear;
            m_data[1] = 0.2126*r_linear + 0.7152*g_linear + 0.0722*b_linear;
            m_data[2] = 0.0193*r_linear + 0.1192*g_linear + 0.9505*b_linear;
        }

    private:
        double m_data[3];
    };

    class sRGB {
    public:
        double& operator[] (size_t i) { return m_data[i]; }
        const double& operator[] (size_t i) const { return m_data[i]; }

        sRGB(double r, double g, double b) {
            m_data[0] = r;
            m_data[1] = g;
            m_data[2] = b;
        }

        sRGB(const XYZ &xyz) {
            m_data[0] = gamma_transform(3.2406 * xyz[0] - 1.5372*xyz[1] - 0.4986*xyz[2]);
            m_data[1] = gamma_transform(-0.9689 * xyz[0] + 1.8758*xyz[1] + 0.0415*xyz[2]);
            m_data[2] = gamma_transform(0.0557 * xyz[0] - 0.2040*xyz[1] + 1.0570*xyz[2]);
        }

        static double gamma_transform(double intensity) {
            if (intensity <= 0.0031308) {
                return 12.92*intensity;
            }
            else {
                return 1.055*pow(intensity, 1.0 / 2.4) - 0.055;
            }
        }

        static double gamma_itransform(double color) {
            if (color <= 0.04045) {
                return color / 12.92;
            }
            else {
                return pow((color + 0.055) / 1.055, 2.4);
            }
        }

    private:
        double m_data[3];
    };

    inline double gamma_transform(double intensity, double gamma = 2.2, double a = 1.0) {
        return a*pow(intensity, 1.0/gamma);
    }

    inline double gamma_itransform(double color, double gamma = 2.2, double a = 1.0) {
        return pow(color / a, gamma);
    }
}
