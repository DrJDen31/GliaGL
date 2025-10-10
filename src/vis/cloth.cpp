#include <fstream>
#include "cloth.h"
#include "argparser.h"
#include "utils.h"
#include "meshdata.h"

extern MeshData *mesh_data;

// ================================================================================
// ================================================================================

Cloth::Cloth(ArgParser *_args) {
  args =_args;

  // open the file
  std::ifstream istr(std::string(args->path+'/'+args->cloth_file).c_str());
  assert (istr.good());
  std::string token;

  // read in the simulation parameters
  istr >> token >> k_structural; assert (token == "k_structural");  // (units == N/m)  (N = kg*m/s^2)
  istr >> token >> k_shear; assert (token == "k_shear");
  istr >> token >> k_bend; assert (token == "k_bend");
  istr >> token >> damping; assert (token == "damping");
  // NOTE: correction factor == .1, means springs shouldn't stretch more than 10%
  //       correction factor == 100, means don't do any correction
  istr >> token >> provot_structural_correction; assert (token == "provot_structural_correction");
  istr >> token >> provot_shear_correction; assert (token == "provot_shear_correction");

  // the cloth dimensions
  istr >> token >> nx >> ny; 
  assert (token == "m");
  assert (nx >= 2 && ny >= 2);

  // the corners of the cloth
  // (units == meters)
  Vec3f a,b,c,d;
  double x,y,z;
  istr >> token >> x >> y >> z; assert (token == "p");
  a.set(x,y,z);
  istr >> token >> x >> y >> z; assert (token == "p");
  b.set(x,y,z);
  istr >> token >> x >> y >> z; assert (token == "p");
  c.set(x,y,z);
  istr >> token >> x >> y >> z; assert (token == "p");
  d.set(x,y,z);
  
  // fabric weight  (units == kg/m^2)
  // denim ~300 g/m^2
  // silk ~70 g/m^2
  double fabric_weight;
  istr >> token >> fabric_weight; assert (token == "fabric_weight");
  double area = AreaOfTriangle(a,b,c) + AreaOfTriangle(a,c,d);

  // create the particles
  particles = new ClothParticle[nx*ny];
  double mass = area*fabric_weight / double(nx*ny);
  for (int i = 0; i < nx; i++) {
    double x = i/double(nx-1);
    Vec3f ab = float(1-x)*a + float(x)*b;
    Vec3f dc = float(1-x)*d + float(x)*c;
    for (int j = 0; j < ny; j++) {
      double y = j/double(ny-1);
      ClothParticle &p = getParticle(i,j);
      Vec3f abdc = float(1-y)*ab + float(y)*dc;
      p.setOriginalPosition(abdc);
      p.setPosition(abdc);
      p.setVelocity(Vec3f(0,0,0));
      p.setMass(mass);
      p.setFixed(false);
    }
  }

  // parse timestep
  istr >> token >> mesh_data->timestep;
  assert (token == "timestep");
  assert (mesh_data->timestep > 0.0);
  
  // the fixed particles
  while (istr >> token) {
      if (token == "f") {
          int i, j;
          double x, y, z;
          istr >> i >> j >> x >> y >> z;
          ClothParticle& p = getParticle(i, j);
          p.setPosition(Vec3f(x, y, z));
          p.setFixed(true);
      } else {
          assert (token == "w");
          double x, y, z;
          istr >> x >> y >> z;
          wind = Vec3f(x, y, z);
      }
  }

  computeBoundingBox();
}

// ================================================================================

void Cloth::computeBoundingBox() {
  box = BoundingBox(getParticle(0,0).getPosition());
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      box.Extend(getParticle(i,j).getPosition());
      box.Extend(getParticle(i,j).getOriginalPosition());
    }
  }
}

// ================================================================================



void Cloth::Animate() {


    // *********************************************************************  
    // ASSIGNMENT:
    //
    // Compute the forces on each particle, and update the state
    // (position & velocity) of each particle.
    //
    // Also, this is where you'll put the Provot correction for super-elasticity
    //
    // ********************************************************************* 
  

    // Eulers:
    //  timestep h,
    //  X = <x, v>
    //  Xf = Xi + h * f(Xi, t)
    //  f(X, t) = <v, ((1/m) * F(x, v, t))>
    //
    //  ==> xf = xi + (h * vi)
    //  ==> vf = vi + ((1/m) * F(xi, vi, t)>
       
   
    // Force of a spring: F(pi, pj) = K(L0 - |pi-pj|) * ((pi-pj) / (|pi-pj|)) 

    // each spring
    std::vector<std::pair<std::pair<int, int>, double>> springs = {
        std::make_pair(std::make_pair(1, 0), k_structural),
        std::make_pair(std::make_pair(0, 1), k_structural),
        std::make_pair(std::make_pair(-1, 0), k_structural),
        std::make_pair(std::make_pair(0, -1), k_structural),
        std::make_pair(std::make_pair(2, 0), k_bend),
        std::make_pair(std::make_pair(0, 2), k_bend),
        std::make_pair(std::make_pair(-2, 0), k_bend),
        std::make_pair(std::make_pair(0, -2), k_bend),
        std::make_pair(std::make_pair(1, 1), k_shear),
        std::make_pair(std::make_pair(1, -1), k_shear),
        std::make_pair(std::make_pair(-1, 1), k_shear),
        std::make_pair(std::make_pair(-1, -1), k_shear)
    };

    // structural springs
    std::vector<std::pair<int, int>> structural = {
        std::make_pair(1, 0),
        std::make_pair(0, 1),
        std::make_pair(-1, 0),
        std::make_pair(0, -1),
    };

    // shear springs
    std::vector<std::pair<int, int>> shear = {
        std::make_pair(1, 1),
        std::make_pair(1, -1),
        std::make_pair(-1, 1),
        std::make_pair(-1, -1)
    };

    // each face surrounding the particle, used for wind normal calculation
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> wind_faces = {
        std::make_pair(std::make_pair(0, 1), std::make_pair(1, 1)),
        std::make_pair(std::make_pair(1, 1), std::make_pair(1, 0)),
        std::make_pair(std::make_pair(1, 0), std::make_pair(1, -1)),
        std::make_pair(std::make_pair(1, -1), std::make_pair(0, -1)),
        std::make_pair(std::make_pair(0, -1), std::make_pair(-1, -1)),
        std::make_pair(std::make_pair(-1, -1), std::make_pair(-1, 0)),
        std::make_pair(std::make_pair(-1, 0), std::make_pair(-1, 1)),
        std::make_pair(std::make_pair(-1, 1), std::make_pair(0, 1))
    };

    //double spring_damping = -1;

    // adaptive timestep
    bool reset = true;
    double max_timestep = 10.f;
    double min_timestep = 0.00000000001f;
    bool reached_ts_min = (mesh_data->timestep < min_timestep);
    bool reached_ts_max = (mesh_data->timestep > max_timestep);

    while (reset) {
        // adaptive timestep
        reset = false;
        bool increase_time = false;
        double max_adjustment = 0;
        double min_allowed_adjustment = 0.0001f;
        double max_allowed_adjustment = 0.1;

        // store previous states in case of timestep adjustment
        std::vector<std::pair<std::pair<int, int>, Vec3f>> old_accels;

        // calculate force
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                if (getParticle(i, j).isFixed()) { continue; }

                // sum all the forces acting on the point
                Vec3f force(0, 0, 0);

                // testing var for adaptive time-step
                double total_length = 0;
                int len_count = 0;

                // if spring exists, calculate its force
                int force_count = 0;
                for (std::vector<std::pair<std::pair<int, int>, double>>::iterator itr = springs.begin(); itr != springs.end(); itr++) {
                    int x = i + itr->first.first;
                    int y = j + itr->first.second;
                    if (x >= 0 && x < nx && y >= 0 && y < ny) {
                        // Force of a spring: F(pi, pj) = K(L0 - ||pi-pj||) * ((pi-pj) / (||pi-pj||)) 
                        double k = itr->second;
                        double l0 = (getParticle(i, j).getOriginalPosition() - getParticle(x, y).getOriginalPosition()).Length();
                        Vec3f diff = (getParticle(i, j).getPosition() - getParticle(x, y).getPosition());
                        double length = diff.Length();
                        total_length += length;
                        len_count++;
                        Vec3f cur_force = (k * (l0 - length) * (Vec3f(diff.x() / length, diff.y() / length, diff.z() / length)));

                        // ALTERNATIVE DAMPING IMPLEMENTATION - PROPORTIONAL BY SPRING
                        // damping -- proportionally resist spring forces in the direction of the current velocity scaled by the velocity
                        //Vec3f v = getParticle(i, j).getVelocity();
                        //double dot = (v.x() * cur_force.x()) + (v.y() * cur_force.y()) + (v.z() * cur_force.z());
                        //Vec3f unit = cur_force;
                        //unit.Normalize();
                        //cur_force += (spring_damping * dot * unit);

                        // set force
                        force += cur_force;
                        getParticle(i, j).setForce(force_count, cur_force);
                    }
                    else {
                        // no force acting from non existent springs, still set up to model it
                        getParticle(i, j).setForce(force_count, Vec3f(0, 0, 0));
                    }
                    force_count++;
                }

                // gravity
                force += getParticle(i, j).getMass() * Vec3f(mesh_data->gravity.data[0], mesh_data->gravity.data[1], mesh_data->gravity.data[2]);

                // wind force
                if (wind.Length() > 0) {
                    // calculate particle normal by averaging nearby face normals (similar to gouraud)
                    Vec3f normal(0, 0, 0);
                    for (std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>::iterator itr = wind_faces.begin(); itr != wind_faces.end(); itr++) {
                        int x1 = i + itr->first.first;
                        int y1 = j + itr->first.second;
                        int x2 = i + itr->second.first;
                        int y2 = j + itr->second.second;
                        if (x1 >= 0 && x1 < nx && y1 >= 0 && y1 < ny && x2 >= 0 && x2 < nx && y2 >= 0 && y2 < ny) {
                            Vec3f e1 = getParticle(i, j).getPosition() - getParticle(x1, y1).getPosition();
                            Vec3f e2 = getParticle(i, j).getPosition() - getParticle(x2, y2).getPosition();
                            Vec3f cross;
                            Vec3f::Cross3(cross, e1, e2);
                            normal += cross;
                        }
                    }
                    normal.Normalize();

                    // wind force is dot of normal and wind vector in the direction of the normal multiplied by a scalar (for nicer file values)
                    Vec3f wf = (((normal.x() * wind.x()) + (normal.y() * wind.y()) + (normal.z() * wind.z())) * normal) * 0.01f;
                    getParticle(i, j).setWindForce(wf);

                    force += wf;
                }

                // general movement damping - helps the cloth settle - used for alternative implementation
                // force += getParticle(i, j).getVelocity() * (-0.005);

                // built-in damping
                force += getParticle(i, j).getVelocity() * (-1 * damping);

                // check if timestep is valid -- if the maximum change was greater than some threshold, timestep likely too high
                double average_length = total_length / len_count;
                Vec3f next_accel = force * (1.f / getParticle(i, j).getMass());
                Vec3f next_vel = getParticle(i, j).getVelocity() + (mesh_data->timestep * next_accel);
                double delta_p = (next_vel * mesh_data->timestep).Length();
                double adjustment = delta_p / average_length;
                if (adjustment > max_allowed_adjustment && !reached_ts_min) {
                    reset = true;
                    break;
                } else if (adjustment > max_adjustment) {
                    max_adjustment = adjustment;
                }

                // store old value in case time step adjustment is made
                old_accels.push_back(std::make_pair(std::make_pair(i, j), getParticle(i, j).getAcceleration()));

                // update values
                getParticle(i, j).setNetForce(force);
                getParticle(i, j).setAcceleration(next_accel);
            }
            if (reset) break;
        }

        // small adjustments, can increase time step -- INSTABLE AND REMOVED (with "&& 0")
        if (max_adjustment < min_allowed_adjustment && !reached_ts_max && 0) {
            reset = true;
            increase_time = true;
        }

        // need to adjust time step
        if (reset) {
            // reset values
            for (std::vector<std::pair<std::pair<int, int>, Vec3f>>::iterator itr = old_accels.begin(); itr != old_accels.end(); itr++) {
                getParticle(itr->first.first, itr->first.second).setAcceleration(itr->second);
            }

            // adapt timestep
            if (increase_time) {
                std::cout << "timestep doubled:  " << mesh_data->timestep << " -> ";
                mesh_data->timestep *= 2.0;
                std::cout << mesh_data->timestep << std::endl;
            } else {
                std::cout << "timestep halved:  " << mesh_data->timestep << " -> ";
                mesh_data->timestep /= 2.0;
                std::cout << mesh_data->timestep << std::endl;
            }

            // max value
            reached_ts_max = mesh_data->timestep > max_timestep;
            reached_ts_min = mesh_data->timestep < min_timestep;
        }
    }

    // update state
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            // calculate new values and update (for non-fixed particles)
            if (!getParticle(i, j).isFixed()) {
                getParticle(i, j).setPosition(getParticle(i, j).getPosition() + (mesh_data->timestep * getParticle(i, j).getVelocity()));
                getParticle(i, j).setVelocity(getParticle(i, j).getVelocity() + (mesh_data->timestep * getParticle(i, j).getAcceleration()));
            }
        }
    }

    // fix positions until no adjustments need to be made
    bool made_adjustment = true;
    int adjustments = 0;
    int rounds = 0;

    while (made_adjustment) {
        made_adjustment = false;

        // for every particle
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {

                // structural correction
                if (provot_structural_correction != 100) {
                    for (std::vector<std::pair<int, int>>::iterator itr = structural.begin(); itr != structural.end(); itr++) {
                        int x = i + itr->first;
                        int y = j + itr->second;
                        if (x >= 0 && x < nx && y >= 0 && y < ny && !getParticle(i, j).isFixed()) {
                            double l0 = (getParticle(i, j).getOriginalPosition() - getParticle(x, y).getOriginalPosition()).Length();
                            Vec3f diff = (getParticle(i, j).getPosition() - getParticle(x, y).getPosition());
                            double length = diff.Length();
                            diff.Normalize();
                            // if the length is over the threshold
                            if (length > ((1.0f + provot_structural_correction + 0.001) * l0)) {
                                // adjust position of one or both points
                                if (getParticle(x, y).isFixed()) {
                                    Vec3f new_pos = getParticle(x, y).getPosition() + ((1.0f + provot_structural_correction) * l0 * diff);
                                    getParticle(i, j).setPosition(new_pos);
                                }
                                else {
                                    Vec3f mid = (getParticle(x, y).getPosition() + getParticle(i, j).getPosition()) * 0.5f;

                                    Vec3f new_pos = mid + (l0 * ((0.5f * (1.0f + provot_structural_correction)) - 0.001f) * diff);
                                    diff.Negate();
                                    Vec3f other_pos = mid + (l0 * ((0.5f * (1.0f + provot_structural_correction)) - 0.001f) * diff);
                                    getParticle(i, j).setPosition(new_pos);
                                    getParticle(x, y).setPosition(other_pos);
                                }
                                made_adjustment = true;
                                adjustments++;
                            }
                        }
                    }
                }

                // shear correction
                if (provot_shear_correction != 100) {
                    for (std::vector<std::pair<int, int>>::iterator itr = shear.begin(); itr != shear.end(); itr++) {
                        int x = i + itr->first;
                        int y = j + itr->second;
                        if (x >= 0 && x < nx && y >= 0 && y < ny && !getParticle(i, j).isFixed()) {
                            double l0 = (getParticle(i, j).getOriginalPosition() - getParticle(x, y).getOriginalPosition()).Length();
                            Vec3f diff = (getParticle(i, j).getPosition() - getParticle(x, y).getPosition());
                            double length = diff.Length();
                            diff.Normalize();
                            // if the length is over the threshold
                            if (length > ((1.0f + provot_shear_correction + 0.001) * l0)) {
                                // adjust position of one or both points
                                if (getParticle(x, y).isFixed()) {
                                    Vec3f new_pos = getParticle(x, y).getPosition() + ((1.0f + provot_shear_correction) * l0 * diff);
                                    getParticle(i, j).setPosition(new_pos);
                                }
                                else {
                                    Vec3f mid = (getParticle(x, y).getPosition() + getParticle(i, j).getPosition()) * 0.5f;

                                    Vec3f new_pos = mid + (l0 * ((0.5f * (1.0f + provot_shear_correction)) - 0.001f) * diff);
                                    diff.Negate();
                                    Vec3f other_pos = mid + (l0 * ((0.5f * (1.0f + provot_shear_correction)) - 0.001f) * diff);
                                    getParticle(i, j).setPosition(new_pos);
                                    getParticle(x, y).setPosition(other_pos);
                                }
                                made_adjustment = true;
                                adjustments++;
                            }
                        }
                    }
                }
            }
        }
        rounds++;
    }
}
