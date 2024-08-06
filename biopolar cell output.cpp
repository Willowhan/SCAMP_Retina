#include <scamp5.hpp>

using namespace SCAMP5_PE;
volatile int diffuse_iterations = 5;
volatile int threshold;

int main(){

	// Initialization
	vs_init(); //M0 core 处理器功能：初始化

	// Setup Host GUI
	vs_gui_set_info(VS_M0_PROJECT_INFO_STRING); //M0 core 处理器功能：gui 信息设置

	auto display_1 = vs_gui_add_display("half-scale", 0, 0);
	auto display_2 = vs_gui_add_display("sobel edge", 0, 1);
	//auto display_3 = vs_gui_add_display("laplacian edge", 1, 0);
	vs_gui_add_slider("threshold: ", 0, 255, 127, &threshold); //一个指向变量的指针，当滑块的值变化时，该变量将被自动更新。例如，&threshold



	while (1) {
		vs_frame_loop_control();

		// 
		scamp5_in(E, threshold);

		scamp5_kernel_begin();

		get_image(C);  // 从 [0, 255] 转换到 [127, 255],归一化到[0-127]

		///*where(C);
		//  MOV(R9, FLAG);
		//all();*/
		//
		////get_image(C,F); //[-127,128]
		////add(C, C, F);

		movx(A, C, north);
		movx(B, C, south);
		add(A, A, B);
		movx(D, C, east);
		movx(E, C, west);
		add(D, D, E); //west+east
		add(D, D, A); //west+east+north+south
		add(D, D, C);   //west+east+north+south+image
		mov(E, D);
		scamp5_kernel_end();

		scamp5_kernel_begin();


		scamp5_diffuse(E, diffuse_iterations); //D diffuse滤波  H

		sub(A, D, E);// Bioplor ON
		//sub(A, A, E);// Bioplor ON
		sub(F, E,D);  // Bioplor OFF
		add(A, A, C); // Bioplor ON Enhanced
		add(F, F, C); // Bioplor OFF Enhanced
		sub(E, A, F); // Parvo 
		divq(B, C);
		add(E, E, B);  //Parvo 边缘细节同时增强


		scamp5_kernel_end();
	}

	return 0;
}
