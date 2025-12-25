/**
 * CONVERGIO EDUCATION - CAMERA CAPTURE (DU05)
 *
 * AVFoundation-based camera capture for macOS.
 * Allows students to photograph homework, textbook pages, etc.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// PHOTO CAPTURE DELEGATE (forward declaration)
// ============================================================================

@interface CameraPhotoCaptureDelegate : NSObject <AVCapturePhotoCaptureDelegate>
@property (nonatomic, copy) void (^completionHandler)(NSData *data);
@end

@implementation CameraPhotoCaptureDelegate

- (void)captureOutput:(AVCapturePhotoOutput *)output
    didFinishProcessingPhoto:(AVCapturePhoto *)photo
    error:(NSError *)error {

    if (error) {
        NSLog(@"Camera capture error: %@", error);
        if (self.completionHandler) {
            self.completionHandler(nil);
        }
        return;
    }

    NSData *photoData = [photo fileDataRepresentation];
    if (self.completionHandler) {
        self.completionHandler(photoData);
    }
}

@end

// ============================================================================
// CAMERA AUTHORIZATION
// ============================================================================

/**
 * Check if camera access is authorized.
 * Returns: 1 = authorized, 0 = denied/restricted, -1 = not determined
 */
int camera_check_authorization(void) {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    switch (status) {
        case AVAuthorizationStatusAuthorized:
            return 1;
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            return 0;
        case AVAuthorizationStatusNotDetermined:
        default:
            return -1;
    }
}

/**
 * Request camera access permission.
 * Returns: 1 if granted, 0 if denied
 */
int camera_request_authorization(void) {
    __block int result = 0;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
        result = granted ? 1 : 0;
        dispatch_semaphore_signal(semaphore);
    }];

    // Wait for user response (with timeout)
    dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 60 * NSEC_PER_SEC));

    return result;
}

// ============================================================================
// CAMERA CAPTURE IMPLEMENTATION
// ============================================================================

/**
 * Capture a photo from the default camera.
 * Returns: Path to captured image (caller must free), or NULL on error
 */
char* camera_capture_photo(void) {
    @autoreleasepool {
        // Check/request authorization
        int auth = camera_check_authorization();
        if (auth == -1) {
            printf("\n📷 Camera access required.\n");
            printf("   Please allow camera access when prompted.\n\n");
            if (!camera_request_authorization()) {
                printf("❌ Camera access denied.\n");
                printf("   Go to System Preferences > Security & Privacy > Camera\n");
                printf("   and allow Convergio to access the camera.\n\n");
                return NULL;
            }
        } else if (auth == 0) {
            printf("❌ Camera access denied.\n");
            printf("   Go to System Preferences > Security & Privacy > Camera\n");
            printf("   and allow Convergio to access the camera.\n\n");
            return NULL;
        }

        // Get default video device
        AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        if (!device) {
            printf("❌ No camera found on this device.\n");
            return NULL;
        }

        NSError *error = nil;

        // Create capture session
        AVCaptureSession *session = [[AVCaptureSession alloc] init];
        session.sessionPreset = AVCaptureSessionPresetPhoto;

        // Create device input
        AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
        if (error || !input) {
            printf("❌ Failed to access camera: %s\n",
                   error ? [[error localizedDescription] UTF8String] : "unknown error");
            return NULL;
        }

        if ([session canAddInput:input]) {
            [session addInput:input];
        } else {
            printf("❌ Cannot add camera input to session.\n");
            return NULL;
        }

        // Create photo output
        AVCapturePhotoOutput *photoOutput = [[AVCapturePhotoOutput alloc] init];
        if ([session canAddOutput:photoOutput]) {
            [session addOutput:photoOutput];
        } else {
            printf("❌ Cannot add photo output to session.\n");
            return NULL;
        }

        // Start session
        [session startRunning];

        // Give camera time to warm up (1 second)
        [NSThread sleepForTimeInterval:1.0];

        printf("\n📷 Camera ready! Press Enter to capture photo...\n");
        printf("   (Position your document in front of the camera)\n");
        getchar();

        // Capture photo using delegate
        __block NSData *photoData = nil;
        dispatch_semaphore_t captureSemaphore = dispatch_semaphore_create(0);

        // Create settings for capture
        AVCapturePhotoSettings *settings = [AVCapturePhotoSettings photoSettings];

        // Create delegate
        CameraPhotoCaptureDelegate *delegate = [[CameraPhotoCaptureDelegate alloc] init];
        delegate.completionHandler = ^(NSData *data) {
            photoData = data;
            dispatch_semaphore_signal(captureSemaphore);
        };

        [photoOutput capturePhotoWithSettings:settings delegate:delegate];

        // Wait for capture (with timeout)
        printf("📸 Capturing...\n");
        dispatch_semaphore_wait(captureSemaphore, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC));

        // Stop session
        [session stopRunning];

        if (!photoData) {
            printf("❌ Failed to capture photo.\n");
            return NULL;
        }

        // Save to temporary file in Documents/ConvergioEducation
        NSString *homeDir = NSHomeDirectory();
        NSString *eduDir = [homeDir stringByAppendingPathComponent:@"Documents/ConvergioEducation"];

        // Create directory if needed
        NSFileManager *fm = [NSFileManager defaultManager];
        if (![fm fileExistsAtPath:eduDir]) {
            [fm createDirectoryAtPath:eduDir withIntermediateDirectories:YES attributes:nil error:nil];
        }

        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setDateFormat:@"yyyyMMdd_HHmmss"];
        NSString *timestamp = [formatter stringFromDate:[NSDate date]];
        NSString *filename = [NSString stringWithFormat:@"photo_%@.jpg", timestamp];
        NSString *filepath = [eduDir stringByAppendingPathComponent:filename];

        // Convert to JPEG and save
        NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithData:photoData];
        if (!imageRep) {
            printf("❌ Failed to process photo data.\n");
            return NULL;
        }

        NSDictionary *properties = @{NSImageCompressionFactor: @0.9};
        NSData *jpegData = [imageRep representationUsingType:NSBitmapImageFileTypeJPEG properties:properties];

        if (![jpegData writeToFile:filepath atomically:YES]) {
            printf("❌ Failed to save photo.\n");
            return NULL;
        }

        printf("\n✓ Photo captured and saved!\n");
        printf("  Location: %s\n\n", [filepath UTF8String]);

        // Return path (caller must free)
        return strdup([filepath UTF8String]);
    }
}

// ============================================================================
// C INTERFACE FOR DOCUMENT UPLOAD
// ============================================================================

/**
 * Check if camera is available (C interface).
 */
int education_camera_available(void) {
    @autoreleasepool {
        AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
        if (!device) return 0;

        // Also check authorization
        int auth = camera_check_authorization();
        return (auth == 1 || auth == -1) ? 1 : 0;  // Available if authorized or not yet determined
    }
}

/**
 * Capture photo from camera (C interface).
 * Returns path to captured image, or NULL on error.
 */
char* education_camera_capture(void) {
    return camera_capture_photo();
}
