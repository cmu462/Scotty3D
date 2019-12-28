#include "environment_light.h"

namespace CMU462 {
namespace StaticScene {

	EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
		: envMap(envMap) {
		// TODO: (PathTracer) initialize things here as needed

		// compute the weight of each pixel and store it in a vector
		double theta, d_theta = PI / envMap->h;
		float L;
		std::vector<double> weight(envMap->w * envMap->h); 
		for (int h = 0; h < envMap->h; h++) {
			for (int w = 0; w < envMap->w; w++) {
				// When computing areas corresponding to a pixel,
				// use the value of theta at the pixel centers
				theta = (h + 0.5)*d_theta;
				L = envMap->data[h * envMap->w + w].illum();
				weight[h * envMap->w + w] = L * sinf(theta);
			}
		}

		// find total weight 
		double total_row_weight, total_weight = 0.0;
		for (auto x : weight) total_weight += x;

		// normalize the weight and store the PDF
		pdf_of_rows.resize(envMap->h);
		pdf_in_each_row.resize(envMap->h);
		for (int h = 0; h < envMap->h; h++) {
			pdf_in_each_row[h].resize(envMap->w);
			total_row_weight = 0.0;
			for (int w = 0; w < envMap->w; w++) {
				total_row_weight += weight[h * envMap->w + w];
			}
			pdf_of_rows[h] = total_row_weight / total_weight;
			for (int w = 0; w < envMap->w; w++) {
				pdf_in_each_row[h][w] = weight[h * envMap->w + w] / total_row_weight;
			}
		}

		// calculate CDF based on PDF
		double total_row_pdf, total_pdf = 0.0;
		cdf_of_rows.resize(envMap->h);
		cdf_in_each_row.resize(envMap->h);
		for (int h = 0; h < envMap->h; h++) {
			cdf_in_each_row[h].resize(envMap->w);

			total_pdf += pdf_of_rows[h];
			cdf_of_rows[h] = total_pdf;
			//printf("%f,", cdf_of_rows[h]);
			total_row_pdf = 0.0;
			for (int w = 0; w < envMap->w; w++) {
				total_row_pdf += pdf_in_each_row[h][w];
				cdf_in_each_row[h][w] = total_row_pdf;
			}
		}
		//printf("\n");
	}

	/**
	* given a sorted vector of cumulative distribution and a key,
	* return the index in that vector that has the smallest value
	* that is equal to or larger than the key.
	* Assumption: vector is sorted from 0 to 1
	* key ranges from 0 to 1
	*/
	int EnvironmentLight::my_binary_search(const std::vector<double> &v, double key) const{
		int mid, l = 0, r = v.size()-1;
		while (r - l > 1) {
			mid = (r + l) / 2;
			if (v[mid] < key) l = mid + 1;
			else r = mid;
		}
		if (v[l] >= key) return l;
		else return r;
	}

Spectrum EnvironmentLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	// TODO: (PathTracer) Implement

	/*********************************************
	/* uniform sampling in the enitre sphere
	/* by first sampling in hemisphere
	/* then decide whether to flip the z coodinate
	**********************************************/
	double Xi1 = (double)(std::rand()) / RAND_MAX;
	double Xi2 = (double)(std::rand()) / RAND_MAX;

	double theta = acos(Xi1);
	double phi = 2.0 * PI * Xi2;

	double xs = sinf(theta) * cosf(phi);
	double ys = sinf(theta) * sinf(phi);
	double zs = cosf(theta);

	if ((double)(std::rand()) / RAND_MAX < 0.5) zs *= -1;
	*wi = (xs, ys, zs);
	Ray r(p,*wi);

	*pdf = 1.0 / 4 / PI;
	*distToLight = INF_D;
	return sample_dir(r);


	/************************************************
	/* importance sampling using the inversion method
	*************************************************/
	// first select a "row" of the environment map according
	//int row = my_binary_search(cdf_of_rows, (double)(std::rand()) / RAND_MAX);
	////printf("(%d,", row);
	//double pdf_row = pdf_of_rows[row];
	//int col = my_binary_search(cdf_in_each_row[row], (double)(std::rand()) / RAND_MAX);
	//double pdf_col = pdf_in_each_row[row][col];

	//double theta = PI / (row+0.5) * envMap->h;
	//double phi = 2.0 * PI / (col+0.5) * envMap->w;
	//*wi = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

	//// The pdfs were the relative share out of the individual pixels. 
	//// However the pixel size could vary a lot, we want the relative 
	//// share of the sphere surface area integral (4 PI). So I needed 
	//// to multiply in the number of pixels and divide out 4*PI.
	//*pdf = pdf_row * envMap->h * pdf_col * envMap->w / 4.0 / PI;
	//*distToLight = INF_D;
	////printf("%f)", *pdf);
	//return envMap->data[row*envMap->w + col];
}

Spectrum EnvironmentLight::sample_dir(const Ray& r) const {
	// TODO: (PathTracer) Implement
	// in meshedit mode the green (y) axis is oriented vertically, not z
	double theta = acos(r.d.y); 
	double phi = atan2(r.d.x, r.d.z);
	if (phi < 0) phi += 2 * PI;
	double x = phi / (2 * PI) * envMap->w;
	double y = theta / PI * envMap->h;
	if (x >= 1.0) x--;
	if (y >= 1.0) y--;

	int x0 = int(x);
	int y0 = int(y);
	int x1 = (x0 + 1) % envMap->w;
	int y1 = y0 + 1;
	if (y0 == envMap->h - 1) y1 = y0;
	
	// implement bilinear interpolation
	Spectrum topleft = envMap->data[y0*envMap->w + x0];
	Spectrum topright = envMap->data[y0*envMap->w + x1];
	Spectrum bottomleft = envMap->data[y1*envMap->w + x0];
	Spectrum bottomright = envMap->data[y1*envMap->w + x1];

	Spectrum top = topleft + (topright + topleft * -1) * ((x - x0) / (x1 - x0));
	Spectrum bottom = bottomleft + (bottomright + bottomleft * -1) * ((x - x0) / (x1 - x0));
	Spectrum result = top + (bottom + top * -1) * ((y - y0) / (y1 - y0));
	return result;
	//return envMap->data[y0*envMap->w + x0];
}

}  // namespace StaticScene
}  // namespace CMU462
