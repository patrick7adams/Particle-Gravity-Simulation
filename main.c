#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "render.h"

const float damping_factor = 0.5f;
int print_flag = 0;
int rad_mass_factor = 20;

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

void print_particle(struct Particle p) {
    printf("Particle with radius %f, mass %f:\nPosition: (%f, %f)\nVelocity: (%f, %f)\nAcceleration: (%f, %f)\nForce: (%f, %f)\n", p.radius, p.mass, p.position.x, p.position.y, p.velocity.x, p.velocity.y, p.acceleration.x, p.acceleration.y, p.force.x, p.force.y);
}

float dist(struct Particle v1, struct Particle v2) {
    struct vec2 dist_vec2 = {v1.position.x - v2.position.x, v1.position.y - v2.position.y}; 
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
    float distance = dist(points[p1_index], points[p2_index]);
    distance -= points[p1_index].radius + points[p2_index].radius;
    if (distance <= 0.0f) {
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
    float gravitational_constant = 0.000001f;
    float distance = dist(*p1, *p2);
    float force = gravitational_constant*(*p1).mass*(*p2).mass/(distance*distance);
    float angle = get_angle((*p1).position, (*p2).position);
    
    float force_x = force * cosf(angle);
    float force_y = force * sinf(angle);
    (*p1).force.x += force_x;
    (*p1).force.y += force_y;
}

void apply_constants(struct Particle* p) {
    (*p).acceleration.x = (*p).force.x / (*p).mass;
    (*p).acceleration.y = (*p).force.y / (*p).mass;

    (*p).velocity.x += (*p).acceleration.x;
    (*p).velocity.y += (*p).acceleration.y;
    
    (*p).position.x += (*p).velocity.x;
    (*p).position.y += (*p).velocity.y;

    if ((*p).position.x+(*p).radius > 1.0f) {
        (*p).position.x = 1.0f-(*p).radius; 
        (*p).velocity.x = -1.0f*damping_factor*(*p).velocity.x;
    }if ((*p).position.y+(*p).radius > 1.0f) {
        (*p).position.y = 1.0f-(*p).radius; 
        (*p).velocity.y = -1.0f*damping_factor*(*p).velocity.y;
    } if ((*p).position.x-(*p).radius < -1.0f) {
        (*p).position.x = -1.0f+(*p).radius; 
        (*p).velocity.x = -1.0f*damping_factor*(*p).velocity.x;
    } if ((*p).position.y-(*p).radius < -1.0f) {
        (*p).position.y = -1.0f+(*p).radius; 
        (*p).velocity.y = -1.0f*damping_factor*(*p).velocity.y;
    }
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

struct Particle p_init(float mass, float pos_x, float pos_y) {
    struct vec2 position = {pos_x, pos_y};
    struct vec2 empty_vec = {0.0f, 0.0f};
    float radius = sqrt(mass/pi)/rad_mass_factor;
    float vel = 0.01;
    float angle = get_angle(empty_vec, position);
    struct vec2 velocity = {vel*cosf(angle), vel*sinf(angle)};
    
    struct Particle p = {radius, mass, position, velocity, empty_vec, empty_vec};
    return p;
}

void gen_points(int num_points, struct Particle* points) {
    srand(time(NULL));
    for (int i = 0; i < num_points; i++) {
        float x_pos = (float) rand() / (float) (RAND_MAX/2) - 1.0f;
        float y_pos = (float) rand() / (float) (RAND_MAX/2) - 1.0f;
        // printf("(%f, %f)", x_pos, y_pos);
        points[i] = p_init(0.005f, x_pos, y_pos);
    }
}

void inputs(GLFWwindow *window, struct Particle* points, int num_points) {
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !print_flag) {
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
}

int main() {
    GLFWwindow* window = init();
    unsigned int program = programInit();
    unsigned int VAO;

    int num_points = 400;
    struct Particle* points = (struct Particle*) malloc(sizeof(struct Particle)*num_points);
    gen_points(num_points, points);
    // points[0] = p_init(0.2f, 0.0f, 0.0f);
    // points[1] = p_init(0.2f, 0.5f, 0.5f);
    // points[2] = p_init(0.2f, -0.5f, -0.5f);
    // points[3] = p_init(0.2f, 0.5f, -0.5f);
    // points[4] = p_init(0.2f, -0.5f, 0.5f);
    
    float center_x[num_points];
    float center_y[num_points];
    float radii[num_points];
    // iterate(points, num_points);
    // iterate(points, num_points);
    while(!glfwWindowShouldClose(window)) {
        inputs(window, points, num_points);
        iterate(points, &num_points);

        for (int i = 0; i < num_points; i++) {
            center_x[i] = points[i].position.x;
            center_y[i] = points[i].position.y;
            radii[i] = points[i].radius;
        }

        render(window, &VAO, program, num_points, center_x, center_y, radii);
    }
    glfwTerminate();
}