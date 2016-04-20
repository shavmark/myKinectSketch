
#include "ofApp.h"

//file:///C:/Users/mark/Downloads/KinectHIG.2.0.pdf

//C:\Program Files\Microsoft SDKs\Kinect\v2.0_1409\Lib\x64
//bugbug fix path for release and debug, and remove any "mark" dependencies
#pragma comment(lib, "C:\\Program Files\\Microsoft SDKs\\Kinect\\v2.0_1409\\Lib\\x64\\Kinect20.lib")
#pragma comment(lib, "C:\\Program Files\\Microsoft SDKs\\Kinect\\v2.0_1409\\Lib\\x64\\Kinect20.face.lib")
#pragma comment(lib, "C:\\Program Files\\Microsoft SDKs\\Kinect\\v2.0_1409\\Lib\\x64\\Kinect20.fusion.lib")

namespace Software2552 {
	bool Trace::checkPointer2(IUnknown *p, const string&  message, char*file, int line) {
		logVerbose2(message, file, line); // should give some good trace
		if (p == nullptr) {
			logError2("invalid pointer " + message, file, line);
			return false;
		}
		return true;
	}
	bool Trace::checkPointer2(KinectBaseClass *p, const string&  message, char*file, int line) {
		logVerbose2(message, file, line); // should give some good trace
		if (p == nullptr) {
			logError2("invalid pointer " + message, file, line);
			return false;
		}
		return true;
	}
	string Trace::buildString(const string& text, char* file, int line) {
		return text + " " + file + ": " + ofToString(line);
	}
	void Trace::logError2(const string& errorIn, char* file, int line) {
		ofLog(OF_LOG_FATAL_ERROR, buildString(errorIn, file, line));
	}
	void Trace::logError2(HRESULT hResult, const string&  message, char*file, int line) {

		logError2(message + ":  " + ofToHex(hResult), file, line);

	}
	// use when source location is not needed
	void Trace::logTraceBasic(const string& message, char *name) {
		string text = name;
		text += " " + message + ";"; // dirty dump bugbug maybe clean up some day
		ofLog(OF_LOG_NOTICE, text);
	}
	void Trace::logTraceBasic(const string& message) {
#if _DEBUG
		ofLog(OF_LOG_VERBOSE, message); //OF_LOG_VERBOSE could dump a lot but in debug thats what we want?
#else
		ofLog(OF_LOG_NOTICE, message);
#endif
	}
	void Trace::logTrace2(const string& message, char*file, int line) {
		if (ofGetLogLevel() >= OF_LOG_NOTICE) {
			ofLog(OF_LOG_NOTICE, buildString(message, file, line));
		}
	}
	bool Trace::CheckHresult2(HRESULT hResult, const string& message, char*file, int line) {
		if (FAILED(hResult)) {
			if (hResult != E_PENDING) {
				logError2(hResult, message, file, line);
			}
			return true; // error found
		}
		logVerbose2(message, file, line);
		return false; // no error
	}
	// allow wide chars this way, bugbug do we need to make wstring seamless?
	std::string Trace::wstrtostr(const std::wstring &wstr) {
		std::string strTo;
		char *szTo = new char[wstr.length() + 1];
		szTo[wstr.size()] = '\0';
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
		strTo = szTo;
		delete[] szTo;
		return strTo;
	}

	/// <summary>
	/// KinectAudioStream constructor. from msft sdk
	/// </summary>
	KinectAudioStream::KinectAudioStream(IStream *p32BitAudio) :	m_cRef(1),	m_p32BitAudio(p32BitAudio),	m_SpeechActive(false)	{
	}

	/// <summary>
	/// SetSpeechState method
	/// </summary>
	void KinectAudioStream::SetSpeechState(bool state)	{
		m_SpeechActive = state;
	}
	STDMETHODIMP_(ULONG) KinectAudioStream::Release() {
		UINT ref = InterlockedDecrement(&m_cRef);
		if (ref == 0) {
			delete this;
		}
		return ref;
	}
	STDMETHODIMP KinectAudioStream::QueryInterface(REFIID riid, void **ppv) {
		if (riid == IID_IUnknown) {
			AddRef();
			*ppv = (IUnknown*)this;
			return S_OK;
		}
		else if (riid == IID_IStream) {
			AddRef();
			*ppv = (IStream*)this;
			return S_OK;
		}
		else {
			return E_NOINTERFACE;
		}
	}

	/////////////////////////////////////////////
	// IStream methods
	STDMETHODIMP KinectAudioStream::Read(void *pBuffer, ULONG cbBuffer, ULONG *pcbRead)	{
		if (pcbRead == NULL || cbBuffer == NULL)		{
			return E_INVALIDARG;
		}

		HRESULT hr = S_OK;

		// 32bit -> 16bit conversion support
		INT16* p16Buffer = (INT16*)pBuffer;
		int factor = sizeof(float) / sizeof(INT16);

		// 32 bit read support
		float* p32Buffer = new float[cbBuffer / factor];
		byte* index = (byte*)p32Buffer;
		ULONG bytesRead = 0;
		ULONG bytesRemaining = cbBuffer * factor;

		// Speech reads at high frequency - this slows down the process
		int sleepDuration = 50;

		// Speech Service isn't tolerant of partial reads
		while (bytesRemaining > 0)		{
			// Stop returning Audio data if Speech isn't active
			if (!m_SpeechActive)			{
				*pcbRead = 0;
				hr = S_FALSE;
				goto exit;
			}

			hr = m_p32BitAudio->Read(index, bytesRemaining, &bytesRead);
			index += bytesRead;
			bytesRemaining -= bytesRead;

			// All Audio buffers drained - wait for buffers to fill
			if (bytesRemaining != 0)			{
				Sleep(sleepDuration);
			}
		}

		// Convert float value [-1,1] to int16 [SHRT_MIN, SHRT_MAX] and copy to output butter
		for (UINT i = 0; i < cbBuffer / factor; i++)		{
			float sample = p32Buffer[i];

			// Make sure it is in the range [-1, +1]
			if (sample > 1.0f)			{
				sample = 1.0f;
			}
			else if (sample < -1.0f)			{
				sample = -1.0f;
			}

			// Scale float to the range (SHRT_MIN, SHRT_MAX] and then
			// convert to 16-bit signed with proper rounding
			float sampleScaled = sample * (float)SHRT_MAX;
			p16Buffer[i] = (sampleScaled > 0.f) ? (INT16)(sampleScaled + 0.5f) : (INT16)(sampleScaled - 0.5f);
		}

		*pcbRead = cbBuffer;

	exit:
		delete p32Buffer;
		return hr;
	}


	KinectBody::KinectBody(Kinect2552 *pKinect) {
		logVerbose("KinectBody");
		setupKinect(pKinect);
		leftHandState = HandState::HandState_Unknown;
		rightHandState = HandState::HandState_Unknown;
		setTalking(false);
	}

	void BodyItems::setupKinect(Kinect2552 *pKinectIn) {
		pKinect = pKinectIn;
	};

	Kinect2552::Kinect2552():KinectBaseClass(){
		pSensor = nullptr;
		width = 0;
		height = 0;
		pColorReader = nullptr;
		pBodyReader = nullptr;
		pDepthReader = nullptr;
		pDescription = nullptr;
		pDepthSource = nullptr;
		pColorSource = nullptr;
		pBodySource = nullptr;
		pCoordinateMapper = nullptr;
		pBodyIndexSource = nullptr;
		pBodyIndexReader = nullptr;

		// Color Table, gives each body its own color
		colors.push_back(ofColor(255, 0, 0));
		colors.push_back(ofColor(0, 0, 255));
		colors.push_back(ofColor(255, 255, 0));
		colors.push_back(ofColor(0, 255, 255));
		colors.push_back(ofColor(255, 0, 255));
		colors.push_back(ofColor(255, 255, 255));

	}

	Kinect2552::~Kinect2552() {
		if (pSensor) {
			pSensor->Close();
		}
		SafeRelease(pSensor);
		SafeRelease(pColorReader);
		SafeRelease(pBodyReader);
		SafeRelease(pDepthReader);
		SafeRelease(pDescription);
		SafeRelease(pDepthSource);
		SafeRelease(pColorSource);
		SafeRelease(pBodySource);
		SafeRelease(pCoordinateMapper);
		SafeRelease(pBodyIndexSource);
		SafeRelease(pBodyIndexReader);
	}
	void KinectBody::setTalking(int count) { 
		talking = count; 
	}

	bool KinectBody::isTalking() {
		if (talking > 0) {
			--talking;
		}
		return talking > 0;
	}
	void KinectBodies::setup(Kinect2552 *kinectInput) {

		if (usingFaces()) {
			KinectFaces::setup(kinectInput);
		}
		else {
			setupKinect(kinectInput); // skip the base class setup, its not needed here
		}

		if (usingAudio()) {
			audio.setup(kinectInput);
		}

		for (int i = 0; i < getKinect()->personCount; ++i) { 
			shared_ptr<KinectBody>p = std::make_shared<KinectBody>(getKinect());
			if (p) {
				bodies.push_back(p);
			}
		}
	}
	void KinectBody::draw(bool drawface) {

		if (objectValid()) {
			//ofDrawCircle(600, 100, 30);
			
			ColorSpacePoint colorSpacePoint = { 0 };
			//ofDrawCircle(400, 100, 30);
			CameraSpacePoint Position = joints[JointType::JointType_HandLeft].Position;
			DepthSpacePoint out;
			ColorSpacePoint out2;
			HRESULT hResult = getKinect()->depth(1, &Position, 1, &out);
			hResult = getKinect()->color(1, &Position, 1, &out2);
			// fails here
			if (SUCCEEDED(hResult)) {
				//ofDrawCircle(700, 100, 30);
				int x = static_cast<int>(colorSpacePoint.X);
				int y = static_cast<int>(colorSpacePoint.Y);
				if ((x >= 0) && (x < getKinect()->getFrameWidth()) && (y >= 0) && (y < getKinect()->getFrameHeight())) {
					if (leftHandState == HandState::HandState_Open) {
						ofDrawCircle(x, y, 30);
					}
					else if (leftHandState == HandState::HandState_Closed) {
						ofDrawCircle(x, y, 5);
					}
					else if (leftHandState == HandState::HandState_Lasso) {
						ofDrawCircle(x, y, 15);
					}
				}
			}
			colorSpacePoint = { 0 };
			//hResult = getKinect()->getCoordinateMapper()->MapCameraPointToColorSpace(joints[JointType::JointType_HandRight].Position, &colorSpacePoint);
			if (SUCCEEDED(hResult)) {
				int x = static_cast<int>(colorSpacePoint.X);
				int y = static_cast<int>(colorSpacePoint.Y);
				if ((x >= 0) && (x < getKinect()->getFrameWidth()) && (y >= 0) && (y < getKinect()->getFrameHeight())) {
					if (rightHandState == HandState::HandState_Open) {
						ofDrawCircle(x, y, 30);
					}
					else if (rightHandState == HandState::HandState_Closed) {
						ofDrawCircle(x, y, 5);
					}
					else if (rightHandState == HandState::HandState_Lasso) {
						ofDrawCircle(x, y, 15);
					}
				}
			}
			// Joint
			for (int type = 0; type < JointType::JointType_Count; type++) {
				colorSpacePoint = { 0 };
				//getKinect()->getCoordinateMapper()->MapCameraPointToColorSpace(joints[type].Position, &colorSpacePoint);
				int x = static_cast<int>(colorSpacePoint.X);
				int y = static_cast<int>(colorSpacePoint.Y);

				if (joints[type].JointType == JointType::JointType_Head) {
					if (isTalking()) {
						//bugbug todo go to this to get better 3d projection CameraSpacePoint
						ofDrawLine(colorSpacePoint.X+2, colorSpacePoint.Y-5, colorSpacePoint.X + 40, colorSpacePoint.Y-5);
						ofDrawLine(colorSpacePoint.X + 5, colorSpacePoint.Y, colorSpacePoint.X + 45, colorSpacePoint.Y);
						ofDrawLine(colorSpacePoint.X+2, colorSpacePoint.Y +5, colorSpacePoint.X + 40, colorSpacePoint.Y + 5);
					}
				}
				if (!drawface) {
					if ((joints[type].JointType == JointType::JointType_Head) | 
						(joints[type].JointType == JointType::JointType_Neck)) {
						continue;// assume face is drawn elsewhere
					}
				}
				if ((x >= 0) && (x < getKinect()->getFrameWidth()) && (y >= 0) && (y < getKinect()->getFrameHeight())) {
					ofDrawCircle(x, y, 10);
				}

			}

		}

	}
	void KinectBodies::draw() {

		for (int count = 0; count < bodies.size(); count++) {
			ofSetColor(getKinect()->getColor(count));
			bodies[count]->draw(!usingFaces());
		}

		if (usingFaces()) {
			KinectFaces::draw();
		}
	}

	void KinectBodies::update(WriteComms &comms) {
		IBodyFrame* pBodyFrame = nullptr;
		HRESULT hResult = getKinect()->getBodyReader()->AcquireLatestFrame(&pBodyFrame);
		if (!hresultFails(hResult, "AcquireLatestFrame")) {
			IBody* pBody[Kinect2552::personCount] = { 0 };

			hResult = pBodyFrame->GetAndRefreshBodyData(Kinect2552::personCount, pBody);
			if (!hresultFails(hResult, "GetAndRefreshBodyData")) {
				for (int count = 0; count < Kinect2552::personCount; count++) {
					bodies[count]->setTalking(0);
					bodies[count]->setValid(false);
					// breaks here
					BOOLEAN bTracked = false;
					hResult = pBody[count]->get_IsTracked(&bTracked);
					if (SUCCEEDED(hResult) && bTracked) {
						ofxJSONElement data;
						data["id"] = count;
						// Set TrackingID to Detect Face
						// LEFT OFF HERE
						UINT64 trackingId = _UI64_MAX;
						hResult = pBody[count]->get_TrackingId(&trackingId);
						if (hresultFails(hResult, "get_TrackingId")) {
							return;
						}

						setTrackingID(count, trackingId); //bugbug if this fails should we just readJsonValue an error and return?
						if (usingAudio()) {
							// see if any audio there
							audio.getAudioCorrelation();
							if (audio.getTrackingID() == trackingId) {
								audio.setValid(); 
								logTrace("set talking");
								bodies[count]->setTalking();
							}
						}

						// get joints
						hResult = pBody[count]->GetJoints(JointType::JointType_Count, bodies[count]->getJoints());
						if (!hresultFails(hResult, "GetJoints")) {
							// Left Hand State
							hResult = pBody[count]->get_HandLeftState(bodies[count]->leftHand());
							if (hresultFails(hResult, "get_HandLeftState")) {
								return;
							}
							data["left"] = bodies[count]->leftHandState;
							hResult = pBody[count]->get_HandRightState(bodies[count]->rightHand());
							if (hresultFails(hResult, "get_HandRightState")) {
								return;
							}
							data["right"] = bodies[count]->rightHandState;
							// Lean
							hResult = pBody[count]->get_Lean(bodies[count]->lean());
							if (hresultFails(hResult, "get_Lean")) {
								return;
							}
							data["lean"]["x"] = bodies[count]->leanAmount.X;
							data["lean"]["y"] = bodies[count]->leanAmount.Y;
						}
						comms.send(data, "kinect/body");
						for (int i = 0; i < JointType::JointType_Count; ++i) {
							ofxJSONElement data;
							data["joints"][i]["id"] = count;

							CameraSpacePoint position = bodies[count]->joints[i].Position;
							DepthSpacePoint depthSpacePoint;
							ColorSpacePoint colorSpacePoint;
							HRESULT hResult = getKinect()->depth(1, &position, 1, &depthSpacePoint);
							if (hresultFails(hResult, "depth")) {
								break;
							}
							hResult = getKinect()->color(1, &position, 1, &colorSpacePoint);
							if (hresultFails(hResult, "color")) {
								break;
							}
							//bugbug maybe track the last one sent and then only send whats changed
							// then the listener just keeps on data set current
							data["joints"][i]["type"] = bodies[count]->joints[i].JointType;
							data["joints"][i]["tracking"] = bodies[count]->joints[i].TrackingState;
							data["joints"][i]["depth"]["x"] = depthSpacePoint.X;
							data["joints"][i]["depth"]["y"] = depthSpacePoint.Y;
							data["joints"][i]["color"]["x"] = colorSpacePoint.X;
							data["joints"][i]["color"]["y"] = colorSpacePoint.Y;
							data["joints"][i]["cam"]["x"] = position.X;
							data["joints"][i]["cam"]["y"] = position.Y;
							data["joints"][i]["cam"]["z"] = position.Z;
							comms.send(data, "kinect/joints");
						}

						bodies[count]->setValid(true);
					}
				}
			}
			for (int count = 0; count < Kinect2552::personCount; count++) {
				SafeRelease(pBody[count]);
			}
		}

		SafeRelease(pBodyFrame);

		//if (usingFaces()) {
			//aquireFaceFrame();
		//}
	}

	bool Kinect2552::open()
	{
		HRESULT hResult = GetDefaultKinectSensor(&pSensor);
		if (hresultFails(hResult, "GetDefaultKinectSensor")) {
			return false;
		}

		hResult = pSensor->Open();
		if (hresultFails(hResult, "IKinectSensor::Open")) {
			return false;
		}
		
		hResult = pSensor->get_BodyIndexFrameSource(&pBodyIndexSource);
		if (hresultFails(hResult, "get_BodyIndexFrameSource")) {
			return false;
		}

		hResult = pSensor->get_ColorFrameSource(&pColorSource);
		if (hresultFails(hResult, "get_ColorFrameSource")) {
			return false;
		}
		
		hResult = pSensor->get_BodyFrameSource(&pBodySource);
		if (hresultFails(hResult, "get_BodyFrameSource")) {
			return false;
		}

		hResult = pBodyIndexSource->OpenReader(&pBodyIndexReader);
		if (hresultFails(hResult, "pBodyIndexSource OpenReader")) {
			return false;
		}

		hResult = pColorSource->OpenReader(&pColorReader);
		if (hresultFails(hResult, "pColorSource OpenReader")) {
			return false;
		}
		
		hResult = pBodySource->OpenReader(&pBodyReader);
		if (hresultFails(hResult, "pBodySource OpenReader")) {
			return false;
		}

		hResult = pColorSource->get_FrameDescription(&pDescription);
		if (hresultFails(hResult, "get_FrameDescription")) {
			return false;
		}

		pDescription->get_Width(&width);  
		pDescription->get_Height(&height);  

		hResult = pSensor->get_CoordinateMapper(&pCoordinateMapper);
		if (hresultFails(hResult, "get_CoordinateMapper")) {
			return false;
		}

		logTrace("Kinect signed on, life is good.");

		return true;
	}

KinectFaces::KinectFaces() {

	logVerbose("KinectFaces");

	// features are the same for all faces
	features = FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
		| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
		| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
		| FaceFrameFeatures::FaceFrameFeatures_Happy
		| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
		| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
		| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
		| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
		| FaceFrameFeatures::FaceFrameFeatures_LookingAway
		| FaceFrameFeatures::FaceFrameFeatures_Glasses
		| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;



};

KinectFace::~KinectFace() {
}
void KinectFace::cleanup()
{
	// do not call in destructor as pointers are used, call when needed
	SafeRelease(pFaceReader);
	SafeRelease(pFaceSource);
}
KinectFaces::~KinectFaces() {

	for (int i = 0; i < faces.size(); ++i) {
		faces[i]->cleanup();
	}

}
void KinectBodies::setTrackingID(int index, UINT64 trackingId) {
	if (usingFaces() && faces.size() >= index) {
		faces[index]->getFaceSource()->put_TrackingId(trackingId); 
	};
}
// get the face readers
void KinectFaces::setup(Kinect2552 *kinectInput) {

	setupKinect(kinectInput);
	buildFaces();

}

void KinectFace::draw()
{
	if (objectValid()) {
		//ofDrawCircle(400, 100, 30);

		if (faceProperty[FaceProperty_LeftEyeClosed] != DetectionResult_Yes)		{
			ofDrawCircle(leftEye().X-15, leftEye().Y, 10);
		}
		if (faceProperty[FaceProperty_RightEyeClosed] != DetectionResult_Yes)		{
			ofDrawCircle(rightEye().X+15, rightEye().Y, 10);
		}
		ofDrawCircle(nose().X, nose().Y, 5);
		if (faceProperty[FaceProperty_Happy] == DetectionResult_Yes || faceProperty[FaceProperty_Happy] == DetectionResult_Maybe || faceProperty[FaceProperty_Happy] == DetectionResult_Unknown) {
			// smile as much as possible
			ofDrawCurve(mouthCornerLeft().X- 70, mouthCornerLeft().Y-70, mouthCornerLeft().X, mouthCornerRight().Y+30, mouthCornerRight().X, mouthCornerRight().Y+30, mouthCornerRight().X+ 70, mouthCornerRight().Y - 70);
		}
		else {
			float height;
			float offset = 0;
			if (faceProperty[FaceProperty_MouthOpen] == DetectionResult_Yes || faceProperty[FaceProperty_MouthOpen] == DetectionResult_Maybe)
			{
				height = 60.0;
				offset = height/2;
			}
			else			{
				height = 5.0;
				offset = 10;
			}
			if (mouthCornerRight().X > 0) {
				points2String();
			}
			float width = abs(mouthCornerRight().X - mouthCornerLeft().X);
			ofDrawEllipse(mouthCornerLeft().X-5, mouthCornerLeft().Y+ offset, width+5, height);
		}
	}

}
void KinectFaces::draw()
{
	//ofDrawCircle(400, 100, 30);
	for (int count = 0; count < faces.size(); count++) {
		ofSetColor(getKinect()->getColor(count));
		faces[count]->draw();
	}

}
void KinectFaces::ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll)
{
	double x = pQuaternion->x;
	double y = pQuaternion->y;
	double z = pQuaternion->z;
	double w = pQuaternion->w;

	// convert face rotation quaternion to Euler angles in degrees
	*pPitch = static_cast<int>(std::atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / M_PI * 180.0f);
	*pYaw = static_cast<int>(std::asin(2 * (w * y - x * z)) / M_PI * 180.0f);
	*pRoll = static_cast<int>(std::atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / M_PI * 180.0f);
}
void KinectFaces::setTrackingID(int index, UINT64 trackingId) { 
	if (faces.size() < index) {
		logErrorString("not enough faces");
		return;
	}
	faces[index]->pFaceSource->put_TrackingId(trackingId);
};

void  BodyItems::aquireBodyFrame()
{
	IBodyFrame* pBodyFrame = nullptr;
	HRESULT hResult = getKinect()->getBodyReader()->AcquireLatestFrame(&pBodyFrame); // getKinect()->getBodyReader() was pBodyReader
	if (!hresultFails(hResult, "AcquireLatestFrame")) {
		IBody* pBody[Kinect2552::personCount] = { 0 };
		hResult = pBodyFrame->GetAndRefreshBodyData(Kinect2552::personCount, pBody);
		if (!hresultFails(hResult, "GetAndRefreshBodyData")) {
			for (int count = 0; count < Kinect2552::personCount; count++) {
				BOOLEAN bTracked = false;
				hResult = pBody[count]->get_IsTracked(&bTracked);
				if (SUCCEEDED(hResult) && bTracked) {

					// Set TrackingID to Detect Face etc
					UINT64 trackingId = _UI64_MAX;
					hResult = pBody[count]->get_TrackingId(&trackingId);
					if (!hresultFails(hResult, "get_TrackingId")) {
						setTrackingID(count, trackingId);
					}
				}
			}
		}
		for (int count = 0; count < Kinect2552::personCount; count++) {
			SafeRelease(pBody[count]);
		}
		SafeRelease(pBodyFrame);
	}

}

// return true if face found
void KinectFaces::update(WriteComms &comms)
{
	if (faces.size() < Kinect2552::personCount) {
		logErrorString("not enough faces:"+ ofToString(faces.size()));
		return;
	}
	for (int count = 0; count < Kinect2552::personCount; count++) {
		IFaceFrame* pFaceFrame = nullptr;
		HRESULT hResult = faces[count]->getFaceReader()->AcquireLatestFrame(&pFaceFrame); // faces[count].getFaceReader() was pFaceReader[count]
		if (SUCCEEDED(hResult) && pFaceFrame != nullptr) {
			BOOLEAN bFaceTracked = false;
			hResult = pFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
			if (SUCCEEDED(hResult) && bFaceTracked) {
				IFaceFrameResult* pFaceResult = nullptr;
				hResult = pFaceFrame->get_FaceFrameResult(&pFaceResult);
				if (SUCCEEDED(hResult) && pFaceResult != nullptr) {
					logVerbose("aquireFaceFrame");
					
					hResult = pFaceResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, faces[count]->facePoint);
					if (hresultFails(hResult, "GetFacePointsInColorSpace")) {
						return;
					}
					pFaceResult->get_FaceRotationQuaternion(&faces[count]->faceRotation);
					hResult = pFaceResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faces[count]->faceProperty);
					if (hresultFails(hResult, "GetFaceProperties")) {
						return;
					}
					pFaceResult->get_FaceBoundingBoxInColorSpace(&faces[count]->boundingBox);
					for (int i = 0; i < faces.size(); i++) {
						ofxJSONElement data;
						data["face"][i]["id"] = count;
						data["face"][i]["eye"]["left"]["closed"] = faces[i]->faceProperty[FaceProperty_LeftEyeClosed] != DetectionResult_Yes;
						data["face"][i]["eye"]["left"]["x"] = faces[i]->facePoint[FacePointType_EyeLeft].X;
						data["face"][i]["eye"]["left"]["y"] = faces[i]->facePoint[FacePointType_EyeLeft].Y;
						data["face"][i]["eye"]["closed"] = faces[i]->faceProperty[FaceProperty_RightEyeClosed] != DetectionResult_Yes;
						data["face"][i]["eye"]["right"]["x"] = faces[i]->facePoint[FacePointType_EyeRight].X;
						data["face"][i]["eye"]["right"]["y"] = faces[i]->facePoint[FacePointType_EyeRight].Y;
						data["face"][i]["nose"]["x"] = faces[i]->facePoint[FacePointType_Nose].X;
						data["face"][i]["nose"]["y"] = faces[i]->facePoint[FacePointType_Nose].Y;
						data["face"][i]["mouth"]["left"]["x"] = faces[i]->facePoint[FacePointType_MouthCornerLeft].X;
						data["face"][i]["mouth"]["left"]["y"] = faces[i]->facePoint[FacePointType_MouthCornerLeft].Y;
						data["face"][i]["mouth"]["right"]["x"] = faces[i]->facePoint[FacePointType_MouthCornerRight].X;
						data["face"][i]["mouth"]["right"]["y"] = faces[i]->facePoint[FacePointType_MouthCornerRight].Y;
						data["face"][i]["mouth"]["open"] = faces[i]->faceProperty[FaceProperty_MouthOpen] == DetectionResult_Yes || faces[i]->faceProperty[FaceProperty_MouthOpen] == DetectionResult_Maybe;
						data["face"][i]["happy"] = faces[i]->faceProperty[FaceProperty_Happy] == DetectionResult_Yes ||
							faces[i]->faceProperty[FaceProperty_Happy] == DetectionResult_Maybe || faces
							[i]->faceProperty[FaceProperty_Happy] == DetectionResult_Unknown; // try hard to be happy
						comms.send(data, "kinect/face");
					}
					SafeRelease(pFaceResult);
					SafeRelease(pFaceFrame);
					faces[count]->setValid();
					return;

				}
				SafeRelease(pFaceResult);
			}
		}
		SafeRelease(pFaceFrame);
	}
}
void KinectFaces::buildFaces() {
	for (int i = 0; i < Kinect2552::personCount; ++i) {
		shared_ptr<KinectFace> p = make_shared<KinectFace>(getKinect());
		if (p) {
			HRESULT hResult;
			hResult = CreateFaceFrameSource(getKinect()->getSensor(), 0, features, &p->pFaceSource);
			if (hresultFails(hResult, "CreateFaceFrameSource")) {
				// 0x83010001 https://social.msdn.microsoft.com/Forums/en-US/b0d4fb49-5608-49d5-974b-f0044ceac5ca/createfaceframesource-always-returning-error?forum=kinectv2sdk
				return;
			}

			hResult = p->pFaceSource->OpenReader(&p->pFaceReader);
			if (hresultFails(hResult, "face.pFaceSource->OpenReader")) {
				return;
			}
			faces.push_back(p);
		}

	}
}


void KinectAudio::getAudioCommands() {
	unsigned long waitObject = WaitForSingleObject(hSpeechEvent, 0);
	if (waitObject == WAIT_TIMEOUT) {
		logVerbose("signaled");
	}
	else if (waitObject == WAIT_OBJECT_0) {
		logTrace("nonsignaled");
		// Retrieved Event
		const float confidenceThreshold = 0.3f;
		SPEVENT eventStatus;
		//eventStatus.eEventId = SPEI_UNDEFINED;
		unsigned long eventFetch = 0;
		pSpeechContext->GetEvents(1, &eventStatus, &eventFetch);
		while (eventFetch > 0) {
			switch (eventStatus.eEventId) {
				// Speech Recognition Events
				//   SPEI_HYPOTHESIS  : Estimate
				//   SPEI_RECOGNITION : Recognition
			case SPEI_HYPOTHESIS:
			case SPEI_RECOGNITION:
				if (eventStatus.elParamType == SPET_LPARAM_IS_OBJECT) {
					// Retrieved Phrase
					ISpRecoResult* pRecoResult = reinterpret_cast<ISpRecoResult*>(eventStatus.lParam);
					SPPHRASE* pPhrase = nullptr;
					HRESULT hResult = pRecoResult->GetPhrase(&pPhrase);
					if (!hresultFails(hResult, "GetPhrase")) {
						if ((pPhrase->pProperties != nullptr) && (pPhrase->pProperties->pFirstChild != nullptr)) {
							// Compared with the Phrase Tag in the grammar file
							const SPPHRASEPROPERTY* pSemantic = pPhrase->pProperties->pFirstChild;
							switch (pSemantic->Confidence) {
							case SP_LOW_CONFIDENCE:
								logTrace("SP_LOW_CONFIDENCE: " + Trace::wstrtostr(pSemantic->pszValue));
								break;
							case SP_NORMAL_CONFIDENCE:
								logTrace("SP_NORMAL_CONFIDENCE: " + Trace::wstrtostr(pSemantic->pszValue));
								break;
							case SP_HIGH_CONFIDENCE:
								logTrace("SP_HIGH_CONFIDENCE: " + Trace::wstrtostr(pSemantic->pszValue));
								break;
							}
							if (pSemantic->SREngineConfidence > confidenceThreshold) {
								logTrace("SREngineConfidence > confidenceThreshold: " + Trace::wstrtostr(pSemantic->pszValue));
							}
						}
						CoTaskMemFree(pPhrase);
					}
				}
			}
			pSpeechContext->GetEvents(1, &eventStatus, &eventFetch);
		}
	}
	else {
		logTrace("other");
	}

}
void KinectAudio::update() { 
	getAudioBody();
	getAudioBeam(); 
	getAudioCommands();
}
//http://www.buildinsider.net/small/kinectv2cpp/07
KinectAudio::KinectAudio(Kinect2552 *pKinect) {
	setupKinect(pKinect);
	logVerbose("KinectAudio");
	audioTrackingId = NoTrackingID;
	trackingIndex = NoTrackingIndex;
	angle = 0.0f;
	confidence = 0.0f;
	correlationCount = 0;
	pAudioBeamList=nullptr;
	pAudioBeam = nullptr;
	pAudioStream = nullptr;
	pAudioSource = nullptr;
	pAudioBeamReader = nullptr;
	pSpeechStream = nullptr;
	pSpeechRecognizer = nullptr;
	pSpeechContext = nullptr;
	pSpeechGrammar = nullptr;
	hSpeechEvent = INVALID_HANDLE_VALUE;
	audioStream = nullptr;
}

KinectAudio::~KinectAudio(){
	if (pSpeechRecognizer != nullptr) {
		pSpeechRecognizer->SetRecoState(SPRST_INACTIVE_WITH_PURGE);
	}
	SafeRelease(pAudioSource);
	SafeRelease(pAudioBeamReader);
	SafeRelease(pAudioBeamList);
	SafeRelease(pAudioBeam);
	SafeRelease(pAudioStream);
	SafeRelease(pSpeechStream); 
	SafeRelease(pSpeechRecognizer);
	SafeRelease(pSpeechContext);
	SafeRelease(pSpeechGrammar);
	if (hSpeechEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(hSpeechEvent);
	}
	CoUninitialize();
	if (audioStream != nullptr) {
		audioStream->SetSpeechState(false);
		delete audioStream;
		audioStream = nullptr;
	}

}

void KinectAudio::setup(Kinect2552 *pKinect) {
	
	setupKinect(pKinect);

	HRESULT hResult = getKinect()->getSensor()->get_AudioSource(&pAudioSource);
	if (hresultFails(hResult, "get_AudioSource")) {
		return;
	}

	hResult = pAudioSource->OpenReader(&pAudioBeamReader);
	if (hresultFails(hResult, "IAudioSource::OpenReader")) {
		return;
	}

	hResult = pAudioSource->get_AudioBeams(&pAudioBeamList);
	if (hresultFails(hResult, "IAudioSource::get_AudioBeams")) {
		return;
	}

	hResult = pAudioBeamList->OpenAudioBeam(0, &pAudioBeam);
	if (hresultFails(hResult, "pAudioBeamList->OpenAudioBeam")) {
		return;
	}

	hResult = pAudioBeam->OpenInputStream(&pAudioStream);
	if (hresultFails(hResult, "IAudioSource::OpenInputStream")) {
		return;
	}

	audioStream = new KinectAudioStream(pAudioStream);

	hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (hresultFails(hResult, "CoInitializeEx")) {
		return;
	}

	if (FAILED(setupSpeachStream())) {
		return;
	}
	if (FAILED(createSpeechRecognizer())) {
		return;
	}
	if (FAILED(findKinect())) {
		return;
	}
	if (FAILED(startSpeechRecognition())) {
		return;
	}

}

HRESULT KinectAudio::setupSpeachStream() {

	HRESULT hResult = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), (void**)&pSpeechStream);
	if (hresultFails(hResult, "CoCreateInstance( CLSID_SpStream )")) {
		return hResult;
	}

	WORD AudioFormat = WAVE_FORMAT_PCM;
	WORD AudioChannels = 1;
	DWORD AudioSamplesPerSecond = 16000;
	DWORD AudioAverageBytesPerSecond = 32000;
	WORD AudioBlockAlign = 2;
	WORD AudioBitsPerSample = 16;

	WAVEFORMATEX waveFormat = { AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0 };

	audioStream->SetSpeechState(true);
	hResult = pSpeechStream->SetBaseStream(audioStream, SPDFID_WaveFormatEx, &waveFormat);
	if (hresultFails(hResult, "ISpStream::SetBaseStream")) {
		return hResult;
	}

	return hResult;

}
HRESULT KinectAudio::findKinect() {

	ISpObjectTokenCategory* pTokenCategory = nullptr;
	HRESULT hResult = CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL, __uuidof(ISpObjectTokenCategory), reinterpret_cast<void**>(&pTokenCategory));
	if (hresultFails(hResult, "CoCreateInstance")) {
		return hResult;
	}

	hResult = pTokenCategory->SetId(SPCAT_RECOGNIZERS, false);
	if (hresultFails(hResult, "ISpObjectTokenCategory::SetId()")) {
		SafeRelease(pTokenCategory);
		return hResult;
	}

	IEnumSpObjectTokens* pEnumTokens = nullptr;
	hResult = CoCreateInstance(CLSID_SpMMAudioEnum, nullptr, CLSCTX_ALL, __uuidof(IEnumSpObjectTokens), reinterpret_cast<void**>(&pEnumTokens));
	if (hresultFails(hResult, "CoCreateInstance( CLSID_SpMMAudioEnum )")) {
		SafeRelease(pTokenCategory);
		return hResult;
	}

	// Find Best Token
	const wchar_t* pVendorPreferred = L"VendorPreferred";
	const unsigned long lengthVendorPreferred = static_cast<unsigned long>(wcslen(pVendorPreferred));
	unsigned long length;
	ULongAdd(lengthVendorPreferred, 1, &length);
	wchar_t* pAttribsVendorPreferred = new wchar_t[length];
	StringCchCopyW(pAttribsVendorPreferred, length, pVendorPreferred);

	hResult = pTokenCategory->EnumTokens(L"Language=409;Kinect=True", pAttribsVendorPreferred, &pEnumTokens); //  English "Language=409;Kinect=True"
	if (hresultFails(hResult, "pTokenCategory->EnumTokens")) {
		SafeRelease(pTokenCategory);
		return hResult;
	}

	SafeRelease(pTokenCategory);
	delete[] pAttribsVendorPreferred;

	ISpObjectToken* pEngineToken = nullptr;
	hResult = pEnumTokens->Next(1, &pEngineToken, nullptr);
	if (hresultFails(hResult, "ISpObjectToken Next")) {
		SafeRelease(pTokenCategory);
		return hResult;
	}
	if (hResult == S_FALSE) {
		//note this but continus things will still work, not sure it matters with the new sdk
		logVerbose("Kinect not found");
	}
	SafeRelease(pEnumTokens);
	SafeRelease(pTokenCategory);

	// Set Speech Recognizer
	hResult = pSpeechRecognizer->SetRecognizer(pEngineToken);
	if (hresultFails(hResult, "SetRecognizer")) {
		return hResult;
	}
	SafeRelease(pEngineToken);

	hResult = pSpeechRecognizer->CreateRecoContext(&pSpeechContext);
	if (hresultFails(hResult, "CreateRecoContext")) {
		return hResult;
	}

	hResult = pSpeechRecognizer->SetPropertyNum(L"AdaptationOn", 0);
	if (hresultFails(hResult, "SetPropertyNum")) {
		return hResult;
	}

	return hResult;

}
HRESULT KinectAudio::createSpeechRecognizer()
{
	// Create Speech Recognizer Instance
	HRESULT hResult = CoCreateInstance(CLSID_SpInprocRecognizer, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), (void**)&pSpeechRecognizer);
	if (hresultFails(hResult, "CLSID_SpInprocRecognizer")) {
		return hResult;
	}

	// Set Input Stream
	hResult = pSpeechRecognizer->SetInput(pSpeechStream, TRUE);
	if (hresultFails(hResult, "pSpeechRecognizer->SetInput")) {
		return hResult;
	}

	return hResult;
}

HRESULT KinectAudio::startSpeechRecognition()
{
	HRESULT hResult = pSpeechContext->CreateGrammar(1, &pSpeechGrammar);
	if (hresultFails(hResult, "CreateGrammar")) {
		return hResult;
	}

	hResult = pSpeechGrammar->LoadCmdFromFile(L"data\\grammar.grxml", SPLO_STATIC); // http://www.w3.org/TR/speech-grammar/ (UTF-8/CRLF)
	if (hresultFails(hResult, "LoadCmdFromFile")) {
		return hResult;
	}

	// Specify that all top level rules in grammar are now active
	hResult = pSpeechGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	// Specify that engine should always be reading audio
	hResult = pSpeechRecognizer->SetRecoState(SPRST_ACTIVE_ALWAYS);

	// Specify that we're only interested in receiving recognition events
	hResult = pSpeechContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

	// Ensure that engine is recognizing speech and not in paused state
	hResult = pSpeechContext->Resume(0);
	if (SUCCEEDED(hResult))	{
		hSpeechEvent = pSpeechContext->GetNotifyEventHandle();
	}
	hSpeechEvent = pSpeechContext->GetNotifyEventHandle();

	return hResult;
}
void  KinectAudio::setTrackingID(int index, UINT64 trackingId) {
	logVerbose("KinectAudio::setTrackingID");

	if (trackingId == audioTrackingId) {
		trackingIndex = index;
		logTrace("set tracking id");
	}
}

// poll kenict to get audo and the body it came from
void KinectAudio::getAudioBody() {
	getAudioCorrelation();
	if (correlationCount != 0) {
		aquireBodyFrame();
	}
}
void KinectAudio::getAudioCorrelation() {
	correlationCount = 0;
	trackingIndex = NoTrackingIndex;
	audioTrackingId = NoTrackingID;

	IAudioBeamFrameList* pAudioBeamList = nullptr;
	HRESULT hResult = getAudioBeamReader()->AcquireLatestBeamFrames(&pAudioBeamList);
	if (!hresultFails(hResult, "getAudioCorrelation AcquireLatestBeamFrames")) {

		IAudioBeamFrame* pAudioBeamFrame = nullptr;
		hResult = pAudioBeamList->OpenAudioBeamFrame(0, &pAudioBeamFrame);
		if (!hresultFails(hResult, "OpenAudioBeamFrame")) {
			IAudioBeamSubFrame* pAudioBeamSubFrame = nullptr;
			hResult = pAudioBeamFrame->GetSubFrame(0, &pAudioBeamSubFrame);
		
			if (!hresultFails(hResult, "GetSubFrame")) {
				hResult = pAudioBeamSubFrame->get_AudioBodyCorrelationCount(&correlationCount);
				
				if (SUCCEEDED(hResult) && (correlationCount != 0)) {
					IAudioBodyCorrelation* pAudioBodyCorrelation = nullptr;
					hResult = pAudioBeamSubFrame->GetAudioBodyCorrelation(0, &pAudioBodyCorrelation);
					
					if (!hresultFails(hResult, "GetAudioBodyCorrelation")) {
						hResult = pAudioBodyCorrelation->get_BodyTrackingId(&audioTrackingId);
						SafeRelease(pAudioBodyCorrelation);
					}
				}
				SafeRelease(pAudioBeamSubFrame);
			}
			SafeRelease(pAudioBeamFrame);
		}
		SafeRelease(pAudioBeamList);
	}

}

// AudioBeam Frame https://masteringof.wordpress.com/examples/sounds/ https://masteringof.wordpress.com/projects-based-on-book/
void KinectAudio::getAudioBeam() {

	IAudioBeamFrameList* pAudioBeamList = nullptr;
	HRESULT hResult = getAudioBeamReader()->AcquireLatestBeamFrames(&pAudioBeamList);
	if (!hresultFails(hResult, "getAudioBeam AcquireLatestBeamFrames")) {
		//bugbug add error handling maybe other clean up
		UINT beamCount = 0;
		hResult = pAudioBeamList->get_BeamCount(&beamCount);
		// Only one audio beam is currently supported, but write the code in case this changes
		for (int beam = 0; beam < beamCount; ++beam) {
			angle = 0.0f;
			confidence = 0.0f;
			IAudioBeamFrame* pAudioBeamFrame = nullptr;
			hResult = pAudioBeamList->OpenAudioBeamFrame(beam, &pAudioBeamFrame);
			
			if (!hresultFails(hResult, "OpenAudioBeamFrame")) {
				// Get Beam Angle and Confidence
				IAudioBeam* pAudioBeam = nullptr;
				hResult = pAudioBeamFrame->get_AudioBeam(&pAudioBeam);
				if (!hresultFails(hResult, "get_AudioBeam")) {
					pAudioBeam->get_BeamAngle(&angle); // radian [-0.872665f, 0.872665f]
					pAudioBeam->get_BeamAngleConfidence(&confidence); // confidence [0.0f, 1.0f]
					SafeRelease(pAudioBeam);
				}
				SafeRelease(pAudioBeamFrame);
			}
		}
		SafeRelease(pAudioBeamList);
	}

}
}