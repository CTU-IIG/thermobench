#include <limits>
#include <iostream>
#include <unistd.h>
#include "model.h"
#include "our_gl.h"
#include <err.h>
#include "../tbwrap.h"

constexpr int width  = 800; // output image size
constexpr int height = 800;

const vec3 light_dir(1,1,1); // light source
const vec3       eye(1,1,3); // camera position
const vec3    center(0,0,0); // camera direction
const vec3        up(0,1,0); // camera up vector

extern mat<4,4> ModelView; // "OpenGL" state matrices
extern mat<4,4> Projection;

bool forever = false;
char *work_done_string = NULL;
unsigned long long work_done = 0;
int work_done_every = 1;
std::vector<Model> models;

struct Shader : IShader {
    const Model &model;
    vec3 l;               // light direction in normalized device coordinates
    mat<2,3> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<3,3> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3,3> ndc_tri;     // triangle in normalized device coordinates

    Shader(const Model &m) : model(m) {
        l = proj<3>((Projection*ModelView*embed<4>(light_dir, 0.))).normalize(); // transform the light vector to the normalized device coordinates
    }

    virtual vec4 vertex(const int iface, const int nthvert) {
        varying_uv.set_col(nthvert, model.uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection*ModelView).invert_transpose()*embed<4>(model.normal(iface, nthvert), 0.)));
        vec4 gl_Vertex = Projection*ModelView*embed<4>(model.vert(iface, nthvert));
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex/gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(const vec3 bar, TGAColor &color) {
        vec3 bn = (varying_nrm*bar).normalize(); // per-vertex normal interpolation
        vec2 uv = varying_uv*bar; // tex coord interpolation

        // for the math refer to the tangent space normal mapping lecture
        // https://github.com/ssloy/tinyrenderer/wiki/Lesson-6bis-tangent-space-normal-mapping
        mat<3,3> AI = mat<3,3>{ {ndc_tri.col(1) - ndc_tri.col(0), ndc_tri.col(2) - ndc_tri.col(0), bn} }.invert();
        vec3 i = AI * vec3(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        vec3 j = AI * vec3(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);
        mat<3,3> B = mat<3,3>{ {i.normalize(), j.normalize(), bn} }.transpose();

        vec3 n = (B * model.normal(uv)).normalize(); // transform the normal from the texture to the tangent space

        double diff = std::max(0., n*l); // diffuse light intensity
        vec3 r = (n*(n*l)*2 - l).normalize(); // reflected light direction, specular mapping is described here: https://github.com/ssloy/tinyrenderer/wiki/Lesson-6-Shaders-for-the-software-renderer
        double spec = std::pow(std::max(r.z, 0.), 5+model.specular(uv)); // specular intensity, note that the camera lies on the z-axis (in ndc), therefore simple r.z

        TGAColor c = model.diffuse(uv);
        for (int i=0; i<3; i++)
            color[i] = std::min<int>(10 + c[i]*(diff + spec), 255); // (a bit of ambient light, diff + spec), clamp the result

        return false; // the pixel is not discarded
    }
};

void print_usage(char *cmd) {
    std::cerr << "Usage: " << cmd << " [-f] [-w work_done_string [-e num]]  obj/model.obj..." << std::endl;
}

void tinyrender_run() {
    std::vector<double> zbuffer(width*height, -std::numeric_limits<double>::max()); // note that the z-buffer is initialized with minimal possible values
    TGAImage framebuffer(width, height, TGAImage::RGB); // the output image
    lookat(eye, center, up);                            // build the ModelView matrix
    viewport(width/8, height/8, width*3/4, height*3/4); // build the Viewport matrix
    projection(-1.f/(eye-center).norm());               // build the Projection matrix

    do {
        for (auto &model : models) { // iterate through all input objects
            Shader shader(model);
            for (int i=0; i<model.nfaces(); i++) { // for every triangle
                vec4 clip_vert[3]; // triangle coordinates (clip coordinates), written by VS, read by FS
                for (int j=0; j<3; j++)
                    clip_vert[j] = shader.vertex(i, j); // call the vertex shader for each triangle vertex
                triangle(clip_vert, shader, framebuffer, zbuffer); // actual rasterization routine call
            }
        }
        if (work_done_string && (work_done++ % work_done_every == 0)) {
            printf("%s=%lld\n", work_done_string, work_done - 1);
            fflush(stdout);
        }
    } while (forever);
    framebuffer.write_tga_file("framebuffer.tga"); // the vertical flip is moved inside the function
}

void tinyrender_init(int argc, char** argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "e:fw:")) != -1) {
        switch (opt) {
        case 'e':
            work_done_every = atoi(optarg);
            if (work_done_every <= 0)
                errx(1, "Usage: -e 'number > 0'");
            break;
        case 'f':
            forever = true;
            break;
        case 'w':
            work_done_string = optarg;
            break;
        default: /* '?' */
            print_usage(argv[0]);
            return;
        }
    }

    if (optind >= argc) {
        print_usage(argv[0]);
        return;
    }

    if (getenv("TB_OPTS") && (forever || work_done_string != NULL || work_done_every != 1))
        errx(1, "-f, -w and -e switches are incompatible with TB_OPTS environment variable");

    if (work_done_string == NULL && work_done_every != 1)
        errx(1, "-e only makes sense with -w");

    for (int m=optind; m<argc; m++) // iterate through all input objects
        models.emplace_back(argv[m]);
}

int main(int argc, char** argv)
{
    tinyrender_init(argc, argv);
    thermobench_wrap(tinyrender_run);
    return 0;
}
