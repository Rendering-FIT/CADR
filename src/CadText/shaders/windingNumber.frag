#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

// push constants
layout(push_constant) uniform pushConstants {
	uint64_t payloadBufferPtr; // one buffer for the whole scene
	uint64_t sceneDataPtr; // one buffer for the whole scene
};

// per-drawable shader data
layout(buffer_reference, std430, buffer_reference_align = 64) restrict readonly buffer ShaderDataRef {
	layout(offset = 0) vec4 color;
	layout(offset = 16) mat4 model;
	layout(offset = 80) uint lineSegmentCount;
	layout(offset = 84) uint curveSegmentCount;
	layout(offset = 88) vec2 padding1;
	layout(offset = 96) vec2 segments[];
};

// scene data
layout(buffer_reference, std430, buffer_reference_align = 64) restrict readonly buffer SceneDataRef {
	uint viewportWidth;
	uint viewportHeight;
	vec2 padding1;
	mat4 view;
	mat4 projection;
};
const uint SceneDataSize = 144;

layout(location = 0) flat in uint64_t inDataPtr;
layout(location = 1) in vec2 fragPosition;

layout(location = 0) out vec4 outColor;

float rayIntersectsLineSegment(vec2 position, vec2 start, vec2 end) {
    // Check if ray and line segment are parallel
    // Winding should be unchanged
    if(start.y == end.y) {
        return 0.f;
    }

    vec2 directionVector = end - start;
    float directionConstant = directionVector.y > 0 ? 1 : -1;

    // Intersection is at line's start point
    if(position.y == start.y) {
        // Check if line segment is pointing up
        // That means the contour defines a fill
        if(directionConstant > 0.f) {
            if(position.x > start.x) {
                // Ray is on the right of line segment
                return 0.f;
            }
            
            // Return winding of 0.5 based on direction vector
            return 0.5f;
        }
        else {
            if(position.x > start.x) {
                // Ray is on the right of line segment
                return 0.f;
            }
            
            // Return winding of -0.5 based on direction vector
            return -0.5f;
        }
    }
    
    // Intersection is at line's end point
    if(position.y == end.y) {
        // Check if line segment is pointing up
        // That means the contour defines a fill
        if(directionConstant > 0.f) {
            if(position.x > end.x) {
                // Ray is on the right of line segment
                return 0.f;
            }
            
            // Return winding of 0.5 based on direction vector
            return 0.5f;
        }
        else {
            if(position.x > end.x) {
                // Ray is on the right of line segment
                return 0.f;
            }
            
            // Return winding of -0.5 based on direction vector
            return -0.5f;
        }
    }

    // Ensure line's start point is below end point
    if(start.y > end.y) {
        vec2 temp = start;
        start = end;
        end = temp;
    }

    // Ray is above or below line
    if(position.y < start.y || position.y > end.y) {
        return 0.f;
    }

    // Check if intersection is to the right of ray
    // If true return winding of 1 or -1 depending on direction vector
    float xIntersection = start.x + (position.y - start.y) * (end.x - start.x) / (end.y - start.y);
    if(abs(position.x - xIntersection) <= 0.1) {
        return 0.f;
    }
    else if(xIntersection > position.x) {
        return directionConstant;
    }

    return 0.f;
}

float getQuadraticDerivativeWinding(float t, float a, float b) {
    float d = 2 * a * t + b;

    if(d > 0) {
        // Curve is pointing up
       return 1;
    }
    else if(d < 0) {
        // Curve is pointing down
        return -1;
    }

    return 0;
}

float getWindingForQuadraticRoot(float t, vec2 position, vec2 start, vec2 control, vec2 end, float a, float b) {
    float winding = 0;
    
    // X coordinate of intersection
    float xIntersection = (1 - t) * (1 - t) * start.x + 2 * (1 - t) * t * control.x + t * t * end.x;

    // Check if intersection is on curve start or end points
    // If true return winding 0.5 or -0.5
    if(position.y == start.y && xIntersection >= position.x && t >= -0.1 && t <= 0.1) {
        return getQuadraticDerivativeWinding(0, a, b) / 2;
    }
    else if(position.y == end.y && xIntersection >= position.x && t >= 0.9 && t <= 1.1) {
        return getQuadraticDerivativeWinding(1, a, b) / 2;
    }

    // Check if root is in range <0, 1>
    // even though start and end points should be handled above
    // because floating point errors occurre its better to check anyway
    // to eliminate certain artefacts
    if(t >= 0 && t <= 1) {
        // Check if intersection is on the right from ray
        if(xIntersection >= position.x) {
            winding += getQuadraticDerivativeWinding(t, a, b);
        }
    }

    return winding;
}

float rayIntersectsCurveSegment(vec2 position, vec2 start, vec2 control, vec2 end) {
    float winding = 0.f;

    // Check if ray is above or below curve
    if(
        (position.y < start.y && position.y < control.y && position.y < end.y) ||
        (position.y > start.y && position.y > control.y && position.y > end.y)
    ) {
        return winding;
    }

    // Quadratic formula coefficients
    float a = start.y - 2 * control.y + end.y;
    float b = 2 * (control.y - start.y);
    float c = start.y - position.y;

    // Curve is actually a line
    if(a == 0) {
        return rayIntersectsLineSegment(position, start, end);
    }

    // Quadratic formula roots
    float sqrtD = sqrt(b * b - 4 * a * c);
    float t0 = (-b + sqrtD) / (2 * a);
    float t1 = (-b - sqrtD) / (2 * a);

    // Compute winding for roots
    winding += getWindingForQuadraticRoot(t0, position, start, control, end, a, b);
    winding += getWindingForQuadraticRoot(t1, position, start, control, end, a, b);

    return winding;
}

void main() {
    // memory pointers
	ShaderDataRef shaderData = ShaderDataRef(inDataPtr);
	SceneDataRef sceneData = SceneDataRef(sceneDataPtr);

    float windingNumber = 0;

    // Compute winding for line segments
    for(uint i = 0; i < 2 * shaderData.lineSegmentCount; i += 2) {
        windingNumber += rayIntersectsLineSegment(fragPosition, shaderData.segments[i], shaderData.segments[i + 1]);
    }
    
    // Compute winding for curve segments
    for(uint i = 2 * shaderData.lineSegmentCount; i < 2 * shaderData.lineSegmentCount + 3 * shaderData.curveSegmentCount; i += 3) {
        windingNumber += rayIntersectsCurveSegment(fragPosition, shaderData.segments[i], shaderData.segments[i + 1], shaderData.segments[i + 2]);
    }

    if(windingNumber != 0) {
        outColor = shaderData.color;
    }
    else {
        discard;
    }
}
