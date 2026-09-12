import cv2
import numpy as np
import math

class WLdetectModel:
    def __init__(self):
        print("Initializing crack width detection model")
    
    def iterated_optimal_incircle_radius_get(self, contours, pixelx, pixely, small_r, big_r, precision):
        """
        Calculate the maximum inscribed circle radius for a contour
        """
        # Implement binary search to find the optimal radius
        while big_r - small_r > precision:
            mid_r = (small_r + big_r) / 2
            if self.is_valid_circle(contours, pixelx, pixely, mid_r):
                small_r = mid_r
            else:
                big_r = mid_r
        return small_r
    
    def is_valid_circle(self, contours, x, y, r):
        """
        Check if a circle with radius r at position (x,y) is inside the contour
        """
        # Create a circle mask
        circle_points = []
        for angle in range(0, 360, 10):  # Check points around the circle
            rad = math.radians(angle)
            px = int(x + r * math.cos(rad))
            py = int(y + r * math.sin(rad))
            circle_points.append([px, py])
        
        # Check if all points are inside the contour
        for point in circle_points:
            result = cv2.pointPolygonTest(contours, (point[0], point[1]), False)
            if result < 0:  # Point is outside the contour
                return False
        return True
    
    def detect_width(self, image_path, contours=None):
        """
        Detect crack width in an image, optionally using provided contours
        """
        # If contours are provided, use them directly
        if contours is not None:
            # Read the image
            img = cv2.imread(image_path)
            if img is None:
                return {
                    "success": False,
                    "error": "Failed to load image",
                    "max_width": 0,
                    "avg_width": 0,
                    "width_image": None
                }
            
            # Process contours to find widths
            return self.process_contours(img, contours, image_path)
        
        # Otherwise, perform contour detection on the image
        # Read the image
        img = cv2.imread(image_path)
        if img is None:
            return {
                "success": False,
                "error": "Failed to load image",
                "max_width": 0,
                "avg_width": 0,
                "width_image": None
            }
        
        # Convert to grayscale
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        
        # Apply threshold
        _, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
        # Find contours
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Process contours to find widths
        return self.process_contours(img, contours, image_path)
        
    def detect_contours(self, image_path, contours):
        """
        DEPRECATED: Use detect_width with contours parameter instead
        """
        return self.detect_width(image_path, contours)
    
    def process_contours(self, img, contours, image_path):
        if not contours:
            return {
                "success": False,
                "error": "No significant cracks detected",
                "max_width": 0,
                "avg_width": 0,
                "width_image": None
            }
        
        # Create output image
        result_img = img.copy()
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) if len(img.shape) > 2 else img
        
        # Calculate width for each contour
        widths = []
        for contour in contours:
            # Get contour mask
            mask = np.zeros_like(gray)
            cv2.drawContours(mask, [contour], 0, 255, -1)
            
            # Get distance transform
            dist_transform = cv2.distanceTransform(mask, cv2.DIST_L2, 5)
            
            # Find maximum distance (half of the width)
            max_dist = np.max(dist_transform)
            widths.append(max_dist * 2)  # Diameter = 2 * radius
            
            # Find coordinates of maximum distance
            y, x = np.where(dist_transform == max_dist)
            if len(x) > 0 and len(y) > 0:
                center = (x[0], y[0])
                radius = int(max_dist)
                
                # Draw circle at maximum width point
                cv2.circle(result_img, center, radius, (0, 0, 255), 2)
                
                # Draw width measurement in pixels
                width_text = f"{max_dist * 2:.1f}px"
                cv2.putText(result_img, width_text, (center[0], center[1] - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        
        # Save result image
        output_path = image_path.replace('.', '_width.')
        cv2.imwrite(output_path, result_img)
        
        # Return results in pixels
        return {
            "success": True,
            "max_width": round(max(widths), 1) if widths else 0,
            "avg_width": round(sum(widths) / len(widths), 1) if widths else 0,
            "width_image": output_path,
            "widths": [round(w, 1) for w in widths]
        }
