
demo: frag.spv vert.spv
	gcc -c evil.c -lvulkan -I. -g
	gcc evil-demo.c evil.o -lvulkan -lxcb -I. -g -o demo.e
.PHONY: demo

frag.spv: frag.glsl
	glslc -fshader-stage=frag $^ -o $@

vert.spv: vert.glsl
	glslc -fshader-stage=vert $^ -o $@

clean:
	rm *.o *.e *.spv
.PHONY: clean
