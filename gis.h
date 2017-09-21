#include <cmath>

namespace gis {
    const double WGS84_a = 6378137.0;
    const double WGS84_b = 6356752.314245;
    const double WGS84_f_inv = 298.257223563;

    const double WGS84_a_over_b = WGS84_a / WGS84_b;
    const double WGS84_b_over_a = WGS84_b / WGS84_a;
    const double WGS84_e = sqrt((WGS84_a*WGS84_a - WGS84_b*WGS84_b) / (WGS84_a*WGS84_a));
    const double WGS84_e_aux = sqrt((WGS84_a*WGS84_a - WGS84_b*WGS84_b) / (WGS84_b*WGS84_b));

    struct ECEF;
    struct LLA;
    struct NED;

    // Earth-Centered, Earth-Fixed (Euclidean but non-inertial, global coord)
    // x: center -> 0 latitude, 0 longitude
    // z: center -> north pole
    // in meters
    struct ECEF {
        double x;
        double y;
        double z;

        // convert to LLA coordinate
        LLA to_lla() const;

        // convert to NED coordinate, with respect to system centered at ref
        NED to_ned(const ECEF& ref) const;
    };

    // Latitude Longitude Altitude of WGS84 (Non-Euclidean, global coord)
    // angles are in radius
    // altitude is in meters
    struct LLA {
        double latitude;
        double longitude;
        double altitude;

        // convert to ECEF coordinate
        ECEF to_ecef() const;

        // convert to NED coordinate, with respect to system centered at ref
        NED to_ned(const LLA& ref) const;
    };

    // North East Down (Euclidean but non-inertial, local coord)
    // in meters
    // Attention: users must take care of the reference frame on their own.
    struct NED {
        double north;
        double east;
        double down;

        // convert to ECEF coordinate, with respect to system centered at ref
        ECEF to_ecef(const ECEF& ref) const;

        // convert to LLA coordinate, with respect to system centered at ref
        LLA to_lla(const LLA& ref) const;
    };

    inline LLA ECEF::to_lla() const {
        LLA result;
        double p = hypot(x, y);
        double theta = atan2(z*WGS84_a, p*WGS84_b);
        double sintheta = sin(theta);
        double costheta = cos(theta);
        result.latitude = atan2(z + WGS84_e_aux*WGS84_e_aux*WGS84_b*sintheta*sintheta*sintheta, p - WGS84_e*WGS84_e*WGS84_a*costheta*costheta*costheta);
        result.longitude = atan2(y, x);
        double sinlat = sin(result.latitude);
        double N = WGS84_a / sqrt(1 - WGS84_e*WGS84_e*sinlat*sinlat);
        result.altitude = p / cos(result.latitude) - N;
        return result;
    }

    inline NED ECEF::to_ned(const ECEF &ref) const {
        NED result;
        LLA ref_lla = ref.to_lla();
        double sinlat = sin(ref_lla.latitude);
        double coslat = cos(ref_lla.latitude);
        double sinlon = sin(ref_lla.longitude);
        double coslon = cos(ref_lla.longitude);
        double dx = x - ref.x;
        double dy = y - ref.y;
        double dz = z - ref.z;
        result.north = -sinlat*coslon*dx - sinlat*sinlon*dy + coslat*dz;
        result.east = -sinlon*dx + coslon*dy;
        result.down = -coslat*coslon*dx - coslat*sinlon*dy - sinlat*dz;
        return result;
    }

    inline ECEF LLA::to_ecef() const {
        ECEF result;
        double coslat = cos(latitude);
        double sinlat = sin(latitude);
        double coslon = cos(longitude);
        double sinlon = sin(longitude);
        double N = WGS84_a / sqrt(1 - WGS84_e*WGS84_e*sinlat*sinlat);
        double R = (N + altitude)*coslat;
        result.x = R*coslon;
        result.y = R*sinlon;
        result.z = (WGS84_b_over_a*WGS84_b_over_a*N + altitude)*sinlat;
        return result;
    }

    inline NED LLA::to_ned(const LLA &ref) const {
        return to_ecef().to_ned(ref.to_ecef());
    }

    inline ECEF NED::to_ecef(const ECEF &ref) const {
        ECEF result;
        LLA ref_lla = ref.to_lla();
        double coslat = cos(ref_lla.latitude);
        double sinlat = sin(ref_lla.latitude);
        double coslon = cos(ref_lla.longitude);
        double sinlon = sin(ref_lla.longitude);
        result.x = ref.x - sinlat*coslon*north - sinlon*east - coslat*coslon*down;
        result.y = ref.y - sinlat*sinlon*north + coslon*east - coslat*sinlon*down;
        result.z = ref.z + coslat*north - sinlat*down;
        return result;
    }

    inline LLA NED::to_lla(const LLA &ref) const {
        return to_ecef(ref.to_ecef()).to_lla();
    }
}
