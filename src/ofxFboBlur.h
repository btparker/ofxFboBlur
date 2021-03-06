/*
 *  ofxFboBlur.h
 *  emptyExample
 *
 *  Created by Oriol Ferrer Mesià on 26/03/12.
 *  Copyright 2012 uri.cat. All rights reserved.
 *
 */

#include "ofMain.h"
#pragma once

#define STRINGIFY(A) #A

class ofxFboBlur{

public:

	ofxFboBlur(){
		blurOffset = 0.5;
		blurPasses = 1;
		numBlurOverlays = 1;
		blurOverlayGain = 128;
	}
	
	void setup(ofFbo::Settings s, bool additive, float scaleDownPercent = 1.0f){

		scaleDown = scaleDownPercent;
		backgroundColor = ofColor(0,0,0,0);

		string fragV = "#extension GL_ARB_texture_rectangle : enable\n" + (string)
		STRINGIFY(
			 uniform float blurLevel;
			 uniform sampler2DRect texture;

			 void main(void){

				 float blurSize = blurLevel;
				 vec4 sum = vec4(0.0);

				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 4.0 * blurSize)) * 0.049049;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 3.0 * blurSize)) * 0.0882;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 2.0 * blurSize)) * 0.1176;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - blurSize)) * 0.147;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.196;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + blurSize)) * 0.147;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 2.0 * blurSize)) * 0.1176;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 3.0 * blurSize)) * 0.0882;
				 sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 4.0 * blurSize)) * 0.049049;
		);

		if(additive) fragV += STRINGIFY( if (sum.a > 0.0) sum.a = 1.0; );

		fragV += STRINGIFY(
				 gl_FragColor = sum;
			 }
		);

		string fragH = "#extension GL_ARB_texture_rectangle : enable\n" + (string)
		STRINGIFY(
				  uniform float blurLevel;
				  uniform sampler2DRect texture;

				  void main(void){

					  float blurSize = blurLevel;
					  vec4 sum = vec4(0.0);

					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x - 4.0 * blurSize, gl_TexCoord[0].y)) * 0.049049;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x - 3.0 * blurSize, gl_TexCoord[0].y)) * 0.0882;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x - 2.0 * blurSize, gl_TexCoord[0].y)) * 0.1176;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x - blurSize, gl_TexCoord[0].y)) * 0.147;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.196;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x + blurSize, gl_TexCoord[0].y)) * 0.147;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x + 2.0 * blurSize, gl_TexCoord[0].y)) * 0.1176;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x + 3.0 * blurSize, gl_TexCoord[0].y)) * 0.0882;
					  sum += texture2DRect(texture, vec2(gl_TexCoord[0].x + 4.0 * blurSize, gl_TexCoord[0].y)) * 0.049049;
					  
					);

		if(additive) fragH += STRINGIFY( if (sum.a > 0.0) sum.a = 1.0; );

		fragH += STRINGIFY(
					  gl_FragColor = sum;
				  }
		);

		string vertex =
		STRINGIFY(
			   void main() {
				   gl_TexCoord[0] = gl_MultiTexCoord0;
				   gl_Position = ftransform();
			   }
		);

		shaderV.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
		shaderV.setupShaderFromSource(GL_FRAGMENT_SHADER, fragV);
		bool ok = shaderV.linkProgram();
		
		shaderH.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
		shaderH.setupShaderFromSource(GL_FRAGMENT_SHADER, fragH);
		ok = shaderH.linkProgram();


		ofLogLevel l = ofGetLogLevel();
		ofSetLogLevel(OF_LOG_WARNING);
		cleanImgFBO.allocate( s );

		s.width *= scaleDown;
		s.height *= scaleDown;

		blurOutputFBO.allocate( s );
		blurTempFBO.allocate( s );
		blurTempFBO2.allocate( s );
		ofSetLogLevel(l);
	}

	void beginDrawScene(){
		cleanImgFBO.begin();
	}

	void endDrawScene(){
		cleanImgFBO.end();
	}

	void performBlur(){
		blur(&cleanImgFBO, &blurOutputFBO, &blurTempFBO, &blurTempFBO2, blurPasses, blurOffset);
	}

	void drawSceneFBO(){
#if (OF_VERSION_MINOR >= 8)
		cleanImgFBO.getTexture().draw(0, 0, cleanImgFBO.getWidth(), cleanImgFBO.getHeight());
#else
		cleanImgFBO.getTexture().draw(0, cleanImgFBO.getHeight(), cleanImgFBO.getWidth(), -cleanImgFBO.getHeight());
#endif
	}

	void drawBlurFbo(bool useCurrentColor = false){
		if(!useCurrentColor) ofSetColor(blurOverlayGain);
		for(int i = 0; i < numBlurOverlays; i++){
			#if (OF_VERSION_MINOR >= 8)
			blurOutputFBO.getTexture().draw(0, 0, blurOutputFBO.getWidth() / scaleDown, blurOutputFBO.getHeight() / scaleDown);
			#else
			blurOutputFBO.getTexture().draw(0, blurOutputFBO.getHeight(), blurOutputFBO.getWidth() / scaleDown, -blurOutputFBO.getHeight() / scaleDown);
			#endif
		}
	}

	void setBackgroundColor(ofColor c){
		backgroundColor = c;
	}

	//access directly please!
	float blurOffset;
	int blurPasses;
	int numBlurOverlays;	
	int blurOverlayGain;	//[0..255]

	ofFbo &getSceneFbo(){return cleanImgFBO;};
	ofFbo &getBlurredSceneFbo(){return blurOutputFBO;};

private:

	float scaleDown;

	void blur( ofFbo * input, ofFbo * output, ofFbo * buffer, ofFbo * buffer2, int iterations, float blurOffset  ){

		if( iterations > 0 ){

			buffer->begin();
			ofClear(backgroundColor);
			buffer->end();

			buffer2->begin();
			ofClear(backgroundColor);
			buffer2->end();

			ofEnableAlphaBlending();
			for (int i = 0; i < iterations; i++) {

				buffer->begin();
					shaderV.begin();
					if (i == 0){ //for first pass, we use input as src; after that, we retro-feed the output of the 1st pass
						shaderV.setUniformTexture( "texture", input->getTexture(), 0 );
					}else{
						shaderV.setUniformTexture( "texture", buffer2->getTexture(), 0 );
					}
					shaderV.setUniform1f("blurLevel", blurOffset * (i + 1) / ( iterations * iterations + 1));
					if (i == 0){
						input->draw(0,0, buffer->getWidth(), buffer->getHeight());
					}else{
						buffer2->draw(0,0);
					}
					shaderV.end();
				buffer->end();

				buffer2->begin();
					shaderH.begin();
					shaderH.setUniformTexture( "texture", buffer->getTexture(), 0 );
					shaderH.setUniform1f("blurLevel", blurOffset * (i + 1) / ( iterations * iterations + 1));
					buffer->draw(0,0);
					shaderH.end();
				buffer2->end();
			}
			//draw back into original fbo

			output->begin();
			ofClear(backgroundColor);
			buffer2->draw(0, 0);
			output->end();

		}else{
			output->begin();
			ofClear(backgroundColor);
			input->draw(0,0, buffer->getWidth(), buffer->getHeight());
			output->end();
		}
	}

	ofFbo	cleanImgFBO;
	ofFbo	blurOutputFBO;
	ofFbo	blurTempFBO;
	ofFbo	blurTempFBO2;

	ofShader shaderV;
	ofShader shaderH;

	ofColor backgroundColor;
};