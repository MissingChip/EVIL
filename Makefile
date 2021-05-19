
demo: frag.spv vert.spv vma.o
	gcc -c evil.c -lvulkan -I. -g
	g++ evil-demo.c evil.o vma.o -lvulkan -lxcb -I. -g -o demo.e
.PHONY: demo

vma.o: vma.cpp
	g++ -c $< -o $@

frag.spv: frag.glsl
	glslc -fshader-stage=frag $^ -o $@

vert.spv: vert.glsl
	glslc -fshader-stage=vert $^ -o $@

clean:
	rm *.o *.e *.spv
.PHONY: clean
