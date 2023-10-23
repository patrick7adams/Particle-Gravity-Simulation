#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "render.h"

const float gravitational_constant = 0.000001f;
const float damping_factor = 0.5f;
const int disable_merging = 0;
int print_flag = 0;
const int rad_mass_factor = 20;
enum collision_modes {
    NO_BORDER = 1,
    SQUARE = 2,
    CIRCLE = 3,
    TELEPORT_CENTER = 4,
    TELEPORT_RANDOM = 5
};
const int collision_mode = NO_BORDER;

enum pointgen_modes {
    RANDOM_STILL = 1,
    RANDOM_VELOCITIES = 2,
    OUTWARDS_VELOCITIES = 3,
    ASTEROID_BELT = 4,
};
const int pointgen_mode = ASTEROID_BELT;
float zoom_factor = 1.0f;

struct vec2 {
    float x;
    float y;
};

float magnitude(struct vec2 v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

struct Particle {
    float radius;
    float mass;
    struct vec2 position;
    struct vec2 velocity;
    struct vec2 acceleration;
    struct vec2 force;
};

struct hcircle {
    float radius;
    struct vec2 position;
};

void print_particle(struct Particle p) {
    printf("Particle with radius %f, mass %f:\nPosition: (%f, %f)\nVelocity: (%f, %f)\nAcceleration: (%f, %f)\nForce: (%f, %f)\n", p.radius, p.mass, p.position.x, p.position.y, p.velocity.x, p.velocity.y, p.acceleration.x, p.acceleration.y, p.force.x, p.force.y);
}

float dist(struct vec2 v1, struct vec2 v2) {
    struct vec2 dist_vec2 = {v1.x - v2.x, v1.y - v2.y}; 
    return magnitude(dist_vec2);
}

struct Particle merge(struct Particle p1, struct Particle p2) {
    struct vec2 empty_vec = {0.0f, 0.0f};
    float mass = p1.mass + p2.mass;
    float radius = sqrt(mass/pi)/rad_mass_factor;
    struct vec2 position;
    if (p1.mass > p2.mass) {
        position.x = p1.position.x;
        position.y = p1.position.y;
    } else if (p1.mass < p2.mass) {
        position.x = p2.position.x;
        position.y = p2.position.y;
    } else {
        position.x = (p1.position.x + p2.position.x) / 2.0f;
        position.y = (p1.position.y + p2.position.y) / 2.0f;
    }
    struct vec2 velocity;
    velocity.x = (p1.mass*p1.velocity.x + p2.mass*p2.velocity.x)/(mass);
    velocity.y = (p1.mass*p1.velocity.y + p2.mass*p2.velocity.y)/(mass);
    struct Particle p = {radius, mass, position, velocity, empty_vec, empty_vec};
    return p;
}

void check_collision(struct Particle* points, int* num_points, int p1_index, int p2_index) {
    float distance = dist(points[p1_index].position, points[p2_index].position);
    distance -= points[p1_index].radius + points[p2_index].radius;
    if (distance <= 0.0f && !disable_merging) {
        // print_particle(points[p1_index]);
        // print_particle(points[p2_index]);
        points[p1_index] = merge(points[p1_index], points[p2_index]);
        // print_particle(points[p1_index]); 
        for (int i = p2_index; i < *num_points-1; i++) {
            points[i] = points[i+1];
        }
        *num_points = *num_points-1;
        struct Particle* new_points = (struct Particle*) realloc(points, *num_points*sizeof(struct Particle));
        points = new_points;
    }
}

float get_angle(struct vec2 p1, struct vec2 p2) {
    return atan2f(p2.y - p1.y, p2.x - p1.x);
}

void set_force(struct Particle* p1, struct Particle* p2) {
    float distance = dist((*p1).position, (*p2).position);
    float force = gravitational_constant*(*p1).mass*(*p2).mass/(distance*distance);
    float angle = get_angle((*p1).position, (*p2).position);
    
    float force_x = force * cosf(angle);
    float force_y = force * sinf(angle);
    (*p1).force.x += force_x;
    (*p1).force.y += force_y;
}

void square_boundary(struct Particle* p) {
    int square_size = 1.0f;
    if ((*p).position.x+(*p).radius > square_size) {
        (*p).position.x = square_size-(*p).radius; 
        (*p).velocity.x = -1.0f*square_size*damping_factor*(*p).velocity.x;
    }if ((*p).position.y+(*p).radius > square_size) {
        (*p).position.y = square_size-(*p).radius; 
        (*p).velocity.y = -1.0f*square_size*damping_factor*(*p).velocity.y;
    } if ((*p).position.x-(*p).radius < -1.0f*square_size) {
        (*p).position.x = -1.0f*square_size+(*p).radius; 
        (*p).velocity.x = -1.0f*square_size*damping_factor*(*p).velocity.x;
    } if ((*p).position.y-(*p).radius < -1.0f*square_size) {
        (*p).position.y = -1.0f*square_size+(*p).radius; 
        (*p).velocity.y = -1.0f*square_size*damping_factor*(*p).velocity.y;
    }
}

void circle_boundary(struct Particle* p) {
    int max_rad = 1.0f;
    struct vec2 empty = {0.0f, 0.0f};
    float distance = dist(empty, (*p).position) + (*p).radius;
    if (distance >= max_rad) {
        float angle = -1.0f*get_angle(empty, (*p).position);
        float diff = distance - max_rad;
        (*p).position.x -= diff * cosf(angle);
        (*p).velocity.x = -1.0f*(*p).velocity.x;
        (*p).position.y += diff * sinf(angle);
        (*p).velocity.y = -1.0f*(*p).velocity.y;

    }
}

void center_teleport(struct Particle* p) {
    int max_rad = 1.0f;
    struct vec2 empty = {0.0f, 0.0f};
    float distance = dist(empty, (*p).position) + (*p).radius;
    if (distance >= max_rad) {
        (*p).position.x = 0;
        (*p).position.y = 0;
    }
}

void random_teleport(struct Particle* p) {
    int max_rad = 1.0f;
    struct vec2 empty = {0.0f, 0.0f};
    float distance = dist(empty, (*p).position) + (*p).radius;
    if (distance >= max_rad) {
        float angle = (float)(rand()%360)*(pi/180);
        float dist = ((float) rand())/RAND_MAX;
        (*p).position.x = dist * cosf(angle);
        (*p).position.y = dist * sinf(angle);
    }
}

void apply_constants(struct Particle* p) {
    (*p).acceleration.x = (*p).force.x / (*p).mass;
    (*p).acceleration.y = (*p).force.y / (*p).mass;

    (*p).velocity.x += (*p).acceleration.x;
    (*p).velocity.y += (*p).acceleration.y;
    
    (*p).position.x += (*p).velocity.x;
    (*p).position.y += (*p).velocity.y;

    if(collision_mode == SQUARE) {square_boundary(p);}
    else if(collision_mode == CIRCLE) {circle_boundary(p);}
    else if(collision_mode == TELEPORT_CENTER) {center_teleport(p);}
    else if(collision_mode == TELEPORT_RANDOM) {random_teleport(p);}
}

void iterate(struct Particle* points, int* num_points) {
    for (int i = 0; i < *num_points; i++) {
        for (int k = 0; k < *num_points; k++) {
            if (i != k) {
                set_force(&points[i], &points[k]);
                check_collision(points, num_points, i, k);
            }
        }
    }
    for(int i = 0; i < *num_points; i++) {
        apply_constants(&points[i]);
        // print_particle(points[i]);
        points[i].force.x = 0.0f;
        points[i].force.y = 0.0f;
    }
}

struct Particle p_init(float mass, struct vec2 position, struct vec2 empty_vec, float vel, float angle) {
    float radius = sqrt(mass/pi)/rad_mass_factor;
    struct vec2 velocity = {vel*cosf(angle), vel*sinf(angle)};
    struct Particle p = {radius, mass, position, velocity, empty_vec, empty_vec};
    return p;
}

void gen_points(int num_points, struct Particle* points) {
    struct vec2 origin = {0.0f, 0.0f};
    for (int i = 0; i < num_points; i++) {
        float x_pos = (float) rand() / (float) (RAND_MAX/2) - 1.0f;
        float y_pos = (float) rand() / (float) (RAND_MAX/2) - 1.0f;
        struct vec2 position = {x_pos, y_pos};
        // printf("(%f, %f)", x_pos, y_pos);
        if (pointgen_mode == RANDOM_STILL) {
            float initial_mass = 0.005f;
            points[i] = p_init(initial_mass, position, origin, 0.0f, 0.0f);
        } else if (pointgen_mode == RANDOM_VELOCITIES) {
            float vel = 0.001 * (rand()%10);
            float angle = (float)(rand()%360)*(pi/180);
            float initial_mass = 0.005f;
            points[i] = p_init(initial_mass, position, origin, vel, angle);
        } else if (pointgen_mode == OUTWARDS_VELOCITIES) {
            float vel = 0.001;
            float initial_mass = 0.005f;
            float angle = get_angle(origin, position);
            points[i] = p_init(initial_mass, position, origin, vel, angle);
        } else if (pointgen_mode == ASTEROID_BELT) {
            float asteroid_mass = 0.005f;
            if (i == 0) {
                float center_point_mass = asteroid_mass * num_points*10;
                float center_point_vel = 0.0f;
                float center_point_angle = 0.0f;
                points[i] = p_init(center_point_mass, origin, origin, center_point_vel, center_point_angle);
            } else {
                // get asteroid pos in belt
                float min_asteroid_radius = 1.7f;
                float max_asteroid_radius = 2.9f;
                float asteroid_gen_angle = (float)(rand()%360)*(pi/180);
                float asteroid_gen_pos = (rand()%1000)*(max_asteroid_radius-min_asteroid_radius)/1000+min_asteroid_radius;
                float asteroid_x = asteroid_gen_pos * cosf(asteroid_gen_angle);
                float asteroid_y = asteroid_gen_pos * sinf(asteroid_gen_angle);
                struct vec2 asteroid_pos = {asteroid_x, asteroid_y};
                // regular stuff
                float dist_from_center = dist(asteroid_pos, points[0].position)-points[0].radius;
                float asteroid_vel = sqrt(gravitational_constant*points[0].mass/dist_from_center)*1.1;
                float asteroid_angle = get_angle(origin, asteroid_pos) + pi/2;
                points[i] = p_init(asteroid_mass, asteroid_pos, origin, asteroid_vel, asteroid_angle);
                print_particle(points[i]);
            }
            
        }
        
    }
}

void inputs(GLFWwindow *window, struct Particle* points, int num_points) {
    float pan_factor = 0.01;
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !print_flag) {
        // Debug key
        for (int i = 0; i < num_points; i++) {
            // print_particle(points[i]);
            printf("Angle between p1 and p2: %f\n", get_angle(points[0].position, points[1].position));
            printf("Angle between p2 and p1: %f", get_angle(points[1].position, points[0].position));
            printf("\n");
            print_flag = 1;
        }
        printf("------------------------------------------\n");
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        print_flag = 0;
    }
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        for(int i = 0; i < num_points; i++) {
            points[i].position.y -= pan_factor;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        for(int i = 0; i < num_points; i++) {
            points[i].position.y += pan_factor;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        for(int i = 0; i < num_points; i++) {
            points[i].position.x -= pan_factor;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        for(int i = 0; i < num_points; i++) {
            points[i].position.x += pan_factor;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        zoom_factor *= 1.01;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        zoom_factor *= 1.0/1.01;
    }

}

int main() {
    srand(time(NULL));
    GLFWwindow* window = init();
    unsigned int program = programInit();
    unsigned int VAO;

    int num_points = 1000;
    struct Particle* points = (struct Particle*) malloc(sizeof(struct Particle)*num_points);
    gen_points(num_points, points);
    // points[0] = p_init(0.2f, 0.0f, 0.0f);
    // points[1] = p_init(0.2f, 0.0f, 0.5f);
    // points[1] = p_init(0.2f, 0.5f, 0.5f);
    // points[2] = p_init(0.2f, -0.5f, -0.5f);
    // points[3] = p_init(0.2f, 0.5f, -0.5f);
    // points[4] = p_init(0.2f, -0.5f, 0.5f);

    int num_h_circles = 0;
    if (collision_mode == CIRCLE || collision_mode == TELEPORT_CENTER || collision_mode == TELEPORT_RANDOM) {
        num_h_circles += 1;
    }
    struct hcircle* hcircles = (struct hcircle*) malloc(sizeof(struct hcircle)*num_h_circles);
    if (num_h_circles > 0) {
        struct vec2 center = {0.0f, 0.0f};
        struct hcircle boundary = {1.0f, center};
        hcircles[0] = boundary;
    }
    
    float center_x[num_points+num_h_circles];
    float center_y[num_points+num_h_circles];
    float radii[num_points+num_h_circles];
    // iterate(points, num_points);
    // iterate(points, num_points);
    while(!glfwWindowShouldClose(window)) {
        inputs(window, points, num_points);
        iterate(points, &num_points);

        for (int i = 0; i < num_points; i++) {
            center_x[i] = points[i].position.x*zoom_factor;
            center_y[i] = points[i].position.y*zoom_factor;
            radii[i] = points[i].radius*zoom_factor;
        }
        for (int i = 0; i < num_h_circles; i++) {
            center_x[num_points+i] = points[i].position.x*zoom_factor;
            center_y[num_points+i] = points[i].position.y*zoom_factor;
            radii[num_points+i] = points[i].radius*zoom_factor;
        }

        render(window, &VAO, program, num_points, num_h_circles, center_x, center_y, radii);
    }
    glfwTerminate();
}