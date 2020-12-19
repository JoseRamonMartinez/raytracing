/* Material.cpp
*
* Realidad Virtual y Aumentada.
*
* Practice 2.
* Ray tracing.
*
* Jose Pascual Molina Masso.
* Escuela Superior de Ingenieria Informatica de Albacete.
*/


#include "glm/glm.hpp" // glm::vec3, glm::dot

#include "Material.h"
#include "light.h"
#include "lightlist.h"
#include "world.h"
#include "limits.h"


/* Constructors */
Material::Material(const glm::vec3 &diff) {

	Ka = glm::vec3(0.0, 0.0, 0.0);
	Kd = diff;
	Kdt = glm::vec3(0.0, 0.0, 0.0);
	Ks = glm::vec3(0.0, 0.0, 0.0);
	Kst = glm::vec3(0.0, 0.0, 0.0);
	n = 0;
	Ie = glm::vec3(0.0, 0.0, 0.0);
	Kr = glm::vec3(0.0, 0.0, 0.0);
	Kt = glm::vec3(0.0, 0.0, 0.0);
	ior = 0.0;
}

Material::Material(const glm::vec3 &amb, const glm::vec3 &diff, const glm::vec3 &diffTrans,
	const glm::vec3 &spec, const glm::vec3 &specTrans, int shine, const glm::vec3 &emis,
	const glm::vec3 &refl, const glm::vec3 &trans, float index) {

	Ka = amb;
	Kd = diff;
	Kdt = diffTrans;
	Ks = spec;
	Kst = specTrans;
	n = shine;
	Ie = emis;
	Kr = refl;
	Kt = trans;
	ior = index;
}

/* Implements the global illumination model */
glm::vec3 Material::Shade(ShadingInfo &shadInfo)
{
	glm::vec3 color(0.0, 0.0, 0.0), V;
	float VdotN, ratio;
	bool isTrans;

	V = -shadInfo.rayDir;
	VdotN = glm::dot(V, shadInfo.normal);
	isTrans = (Kt != glm::vec3(0.0, 0.0, 0.0));
	if (VdotN < 0) {

		// The viewer stares at an interior or back face of the object,
		// we will only illuminate it if material is transparent
		if (isTrans) {
			shadInfo.normal = -shadInfo.normal;  // Reverse normal
			VdotN = -VdotN;
			ratio = 1.0 / ior; // Ray always comes from vacuum (hollow objects)
			//ratio = ior;  // Use this instead for solid objects
		}
		else
			return color;
	}
	else {

		// The viewer stares at a front face of the object
		if (isTrans)
			ratio = 1.0 / ior; // Ray comes from vacuum
	}

	Light *pLuz;

	// Luz ambiental ----- Ia=Ka*sumatorio(Il ambiental)
	bool calculoambiental = (Ka != glm::vec3(0.0, 0.0, 0.0));
	if (calculoambiental) {

		if (shadInfo.pWorld->lights.Length() > 0) {

			glm::vec3 sumatorioIa(0.0, 0.0, 0.0);

			// Accedemos a la lista de luces para sumar las contribuciones de todas ellas
			pLuz = shadInfo.pWorld->lights.First();
			while (pLuz != NULL) {

				sumatorioIa += pLuz->Ia;
				pLuz = shadInfo.pWorld->lights.Next();
			}
			// Le sumamos al color el valor de Ia=Ka*sumatoriaIa
			color += Ka * sumatorioIa;
		}
	}

	/* Luz Difusa y especular */
	bool calculoDifusa = (Kd != glm::vec3(0.0, 0.0, 0.0));
	bool calculoEspecular = (Ks != glm::vec3(0.0, 0.0, 0.0));
	if (calculoDifusa || calculoEspecular) {

		if (shadInfo.pWorld->lights.Length() > 0) {

			//Inicialización sumatorios
			glm::vec3 sumId(0.0, 0.0, 0.0);
			glm::vec3 sumIs(0.0, 0.0, 0.0);
			glm::vec3 sumIdt(0.0, 0.0, 0.0);
			glm::vec3 sumIst(0.0, 0.0, 0.0);


			// Accedemos a la lista de luces
			pLuz = shadInfo.pWorld->lights.First();
			while (pLuz != NULL) {

				glm::vec3 L, R, T;
				float dotN, t;
				Object *objeto;
				glm::vec3 luz(1.0, 1.0, 1.0);

				L = glm::normalize(pLuz->position - shadInfo.point);

				dotN = glm::dot(L, shadInfo.normal);
				if (dotN > 0.0) {  // Luz desde el espectador

					
					objeto = shadInfo.pWorld->objects.First();
					while (objeto != NULL) { // Comprobacion de sombra

						// Rayo Shadow
						t = objeto->NearestInt(shadInfo.point, L);
						if (TMIN < t) {  //self-occlusion

							if (objeto->pMaterial->Kt == glm::vec3(0.0, 0.0, 0.0)) {
								luz = glm::vec3(0.0, 0.0, 0.0);
								break;
							}
							luz *= objeto->pMaterial->Kt;
						}
						objeto = shadInfo.pWorld->objects.Next();
					}
					shadInfo.pWorld->numShadRays++;

					//Si no es shadow-ray
					if (luz != glm::vec3(0.0, 0.0, 0.0)) {  

						if (calculoDifusa)
							sumId += luz * pLuz->Id * dotN;

						if (calculoEspecular) {
							R = (2 * dotN * shadInfo.normal) - L;
							sumIs += luz * pLuz->Is * glm::pow(glm::dot(R, V), n);
						}
					}
				}
				else if (isTrans) { // Comprobar si es Trans=Transparente

					//Comprobar si esta en sombra
					objeto = shadInfo.pWorld->objects.First();
					while (objeto != NULL) {

						//Llamar shadow-ray
						t = objeto->NearestInt(shadInfo.point, L);
						if (TMIN < t) {  //self-occlusion

							if (objeto->pMaterial->Kt == glm::vec3(0.0, 0.0, 0.0)) {
								luz = glm::vec3(0.0, 0.0, 0.0);
								break;
							}
							luz *= objeto->pMaterial->Kt;
						}
						objeto = shadInfo.pWorld->objects.Next();

					}
					shadInfo.pWorld->numShadRays++;

					if (luz != glm::vec3(0.0, 0.0, 0.0)) {  // COmprobar que no es shadow

						if (calculoDifusa)
							sumIdt += luz * pLuz->Id * (-dotN);

						if (calculoEspecular) {

							glm::vec3 T;
							float radical;

							radical = 1.0 + (ratio * ratio) * ((dotN * dotN) - 1.0);
							if (radical > 0.0) {

								T = glm::normalize(ratio*(-L) + (ratio*(-dotN) - sqrt(radical))*(-shadInfo.normal));
								sumIst += luz * pLuz->Is * glm::pow(glm::dot(T, V), n);
							}
						}
					}
				}

				pLuz = shadInfo.pWorld->lights.Next();
			}
			color += (Kd * sumId) + (Ks * sumIs) + (Kdt * sumIdt) + (Kst * sumIst);
		}
	}

	/* Termino  de emision */
	color += Ie;

	/* El límite de la recursividad*/
	if (shadInfo.depth >= shadInfo.pWorld->maxDepth)
		return color;

	//Globales
	/*Reflaccion  */
	if (Kr != glm::vec3(0.0, 0.0, 0.0)) {

		glm::vec3 Rv;

		Rv = (2 * VdotN * shadInfo.normal) - V;
		color += Kr * shadInfo.pWorld->Trace(shadInfo.point, Rv, shadInfo.depth + 1);

		shadInfo.pWorld->numReflRays++;
	}

	/*Rrefraction */
	if (isTrans) {

		glm::vec3 Tv;
		float radical;

		radical = 1.0 + (ratio * ratio) * ((VdotN * VdotN) - 1.0);
		if (radical > 0.0) {

			Tv = glm::normalize(ratio*shadInfo.rayDir + (ratio*VdotN - sqrt(radical))*shadInfo.normal);
			color += Kt * shadInfo.pWorld->Trace(shadInfo.point, Tv, shadInfo.depth + 1);

			shadInfo.pWorld->numRefrRays++;
		}
	}

	return color;
}

	