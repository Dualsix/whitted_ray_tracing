#include <math.h>

/***************************************************************************
*                                                                          *
* This is the source file for a ray tracer. It defines most of the		   *
* fundamental functions using in ray tracing.  In particular, "Trace" and  *
* "Shade".  It also defines the MakeImage function, which casts all the    *
* rays from the eye, and several object intersection methods.  Some of the *
* functions are left blank, or with "place holders" that don't do very     *
* much.  You need to fill these in.                                        *
*                                                                          *
*                                                                          *
***************************************************************************/

static const int tree_depth = 1;		// Number of recursions to compute indirect illumination


#include "Raytracer.h"

// Draw image on the screen
void Raytracer::draw( void )
{
	glDrawPixels( resolutionX, resolutionY, GL_RGB, GL_UNSIGNED_BYTE, &(*I)( 0 , 0 ) );
}

// Cast_line casts all the initial rays starting from the eye for a single
//  raster line. Copies pixels to image object.
void Raytracer::cast_line( World world )
{
    Ray ray;
	Color color;	// Color computed when using multiple rays per pixel

	ray.origin = world.getCamera().eye; // All initial rays originate from the eye.
	ray.no_emitters = false;

    Vec3 G  = Unit( world.getCamera().lookat - world.getCamera().eye );	// Gaze direction.
    Vec3 U  = Unit( world.getCamera().up / G );							// Up vector.
    Vec3 R  = Unit( G ^ U );											// Right vector.
    Vec3 O  = ( world.getCamera().vpdist * G ) - R + U;					// "Origin" for the raster.
    Vec3 dU = U * ( 2.0 / ( resolutionY - 1 ) );						// Up increments.
	Vec3 dR = R * ( 2.0 / ( resolutionX - 1 ) );						// Right increments.

    if( currentLine % 10 == 0 ) cout << "line " << currentLine << endl;
    for( int i = 0; i < resolutionX; i++ )
    {
		ray.direction = Unit( O + i * dR - currentLine * dU  );
		(*I)( resolutionY-currentLine-1, i ) = ToneMap( Trace( ray, world.getScene(), tree_depth ) );
    }

	if (++currentLine == resolutionY)
	{
		// Image computation done, save it to file
		cout << "done." << endl;
	    I->Write( "Resultat.ppm" );
		isDone = true;
	}
}


// This is a trivial tone mapper; it merely maps values that are
// in [0,1] and maps them to integers between 0 and 255.  If the
// real value is above 1, it merely truncates.  A true tone mapper
// would attempt to handle very large values nicely, without
// truncation; that is, it would try to compensate for the fact that
// displays have a very limited dynamic range.
Pixel Raytracer::ToneMap( const Color &color )
{
	int red   = (int)floor( 256 * color.red   );
    int green = (int)floor( 256 * color.green );
    int blue  = (int)floor( 256 * color.blue  );
    channel r = (channel)( red   >= 255 ? 255 : red   ); 
    channel g = (channel)( green >= 255 ? 255 : green ); 
    channel b = (channel)( blue  >= 255 ? 255 : blue  );
    return Pixel( r, g, b );
}

// Trace is the most fundamental of all the ray tracing functions.  It
// answers the query "What color do I see looking along the given ray
// in the current scene?"  This is an inherently recursive process, as
// trace may again be called as a result of the ray hitting a reflecting
// object.  To prevent the possibility of infinite recursion, a maximum
// depth is placed on the resulting ray tree.
Color Raytracer::Trace( const Ray &ray, const Scene &scene, int max_tree_depth  )
{
    Color   color;                    // The color to return.
    HitInfo hitinfo;                  // Holds info to pass to shader.

	// Intitallizes hit distance to infinity to allow finding intersections in all ray length
	hitinfo.geom.distance = Infinity;

	if (Cast( ray, scene, hitinfo ) > 0.0f && max_tree_depth > -1 )
	{
        // The ray hits an object, so shade the point that the ray hit.
        // Cast has put all necessary information for Shade in "hitinfo".
		
		// If the ray has no_emitters activated and the first hit is an emitter
		//  this ray shouldn't contribute to the color of the current pixel
		if( hitinfo.material.Emitter() && ray.no_emitters == true ) color = Color ();

		// The ray hits an object, so shade the point that the ray hit.
        // Cast has put all necessary information for Shade in "hitinfo".
		else color = Shade( hitinfo, scene, max_tree_depth - 1  );
    }
    else
    {
        // Either the ray has failed to hit anything, or
        // the recursion has bottomed out.
        color = scene.bgcolor;
    }
    
    return color;
}

// Cast finds the first point of intersection (if there is one)
// between a ray and a list of geometric objects.  If no intersection
// exists, the function returns false.  Information about the
// closest object hit is returned in "hitinfo". 
int Raytracer::Cast( const Ray &ray, const Scene &scene, HitInfo &hitinfo, Object *ignore )
{
	int hit = false;

    // Each intersector is ONLY allowed to write into the "HitGeom"
    // structure if it has determined that the ray hits the object
    // at a CLOSER distance than currently recorded in HitGeom.distance.
    // When a closer hit is found, the material fields of the "HitInfo"
    // structure are updated to hold the material of the object that 
    // was just hit.

    for( Object *object = scene.first; object != NULL; object = object->next )
    {
        if( object != ignore && object->Intersect( ray, hitinfo.geom ) )
            {
            hitinfo.material = object->material;  // Material of closest surface.
            hit = true;                           // We have hit an object.
            }
    }
    return hit;
}

// Shade assigns a color to a point on a surface, as it is seen
// from another point.  The coordinates of these points, the normal
// of the surface, and the surface material are all recorded in the
// HitInfo structure.  The shader will typically make calls to Trace
// to handle shadows and reflections.

//rellenar
Color Raytracer::Shade( const HitInfo &hit, const Scene &scene, int max_tree_depth )
{
	//direct light
	Color cdirect;
	Color cindirect;
	Color lindirect;
	Color ambient = scene.ambient*hit.material.m_Diffuse;
	cdirect = ambient;
	Vec3 V = Unit(hit.geom.point - hit.geom.origin);
	Vec3 N = hit.geom.normal;
	Ray shadow;
	shadow.origin = hit.geom.point + N*Epsilon;
	shadow.no_emitters = true;
	HitInfo vacio;

	Ray rayo;
	rayo.direction = Reflection(V, N);
	rayo.origin = hit.geom.point + N*Epsilon;
	rayo.no_emitters = true;
	cindirect = Trace(rayo, scene, max_tree_depth); //luz indirecta

	Ray rayor;
	rayor.origin = hit.geom.point - N;

	rayor.direction = Refraction(V, N, hit.material.m_RefractiveIndex);
	rayor.no_emitters = true;
	
	Color refraction = Trace(rayor, scene, max_tree_depth);
	lindirect = cindirect*hit.material.m_Opacity + (1 - hit.material.m_Opacity) * refraction;

	for (int i = 0; i < scene.num_lights; i++) {
		Vec3 L = Unit(scene.light[i].m_Position - hit.geom.point);
		Vec3 R = Reflection(L, N);
		float RV = R*V;
		shadow.direction = Reflection(L, N*Epsilon);
		
		vacio.geom.distance = dist(scene.light[i].m_Position, hit.geom.point);

		if (Cast(shadow, scene, vacio)) { //sombras
			continue;
		}
		float NL = N*L;
		if (NL < 0) {
			NL = 0;
		}
		Color diffuse =  NL * hit.material.m_Diffuse * scene.light[i].m_Color;
		Color specular;
		if (RV > 0 && hit.material.m_Phong_exp > 0) {
			specular = pow(RV, hit.material.m_Phong_exp) * hit.material.m_Specular * scene.light[i].m_Color;
		}
		cdirect += diffuse + specular; //direct light
		
	}
	
	Color totalcolor = cdirect * (1 - hit.material.m_Reflectivity) + lindirect*hit.material.m_Reflectivity;
	return totalcolor;
}
