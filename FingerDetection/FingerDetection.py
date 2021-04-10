import cv2
import numpy as np
from collections import deque

hand_hist = None
traverse_point = []
total_rectangle = 9
hand_rect_one_x = None
hand_rect_one_y = None

hand_rect_two_x = None
hand_rect_two_y = None

def setValues(x):
   print("")

def rescale_frame(frame, wpercent=130, hpercent=130):
    width = int(frame.shape[1] * wpercent / 100)
    height = int(frame.shape[0] * hpercent / 100)
    return cv2.resize(frame, (width, height), interpolation=cv2.INTER_AREA)


def contours(hist_mask_image):
    gray_hist_mask_image = cv2.cvtColor(hist_mask_image, cv2.COLOR_BGR2GRAY)
    ret, thresh = cv2.threshold(gray_hist_mask_image, 0, 255, 0)
    _, cont, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    return cont

def draw_rect(frame):
    rows, cols, _ = frame.shape
    global total_rectangle, hand_rect_one_x, hand_rect_one_y, hand_rect_two_x, hand_rect_two_y

    hand_rect_one_x = np.array(
        [6 * rows / 20, 6 * rows / 20, 6 * rows / 20, 9 * rows / 20, 9 * rows / 20, 9 * rows / 20, 12 * rows / 20,
         12 * rows / 20, 12 * rows / 20], dtype=np.uint32)

    hand_rect_one_y = np.array(
        [9 * cols / 20, 10 * cols / 20, 11 * cols / 20, 9 * cols / 20, 10 * cols / 20, 11 * cols / 20, 9 * cols / 20,
         10 * cols / 20, 11 * cols / 20], dtype=np.uint32)

    hand_rect_two_x = hand_rect_one_x + 10
    hand_rect_two_y = hand_rect_one_y + 10

    for i in range(total_rectangle):
        cv2.rectangle(frame, (hand_rect_one_y[i], hand_rect_one_x[i]),
                      (hand_rect_two_y[i], hand_rect_two_x[i]),
                      (0, 255, 0), 1)

    return frame


def hand_histogram(frame):
    global hand_rect_one_x, hand_rect_one_y

    hsv_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    roi = np.zeros([90, 10, 3], dtype=hsv_frame.dtype)

    for i in range(total_rectangle):
        roi[i * 10: i * 10 + 10, 0: 10] = hsv_frame[hand_rect_one_x[i]:hand_rect_one_x[i] + 10,
                                          hand_rect_one_y[i]:hand_rect_one_y[i] + 10]

    hand_hist = cv2.calcHist([roi], [0, 1], None, [180, 256], [0, 180, 0, 256])
    return cv2.normalize(hand_hist, hand_hist, 0, 255, cv2.NORM_MINMAX)


def hist_masking(frame, hist):
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    dst = cv2.calcBackProject([hsv], [0, 1], hist, [0, 180, 0, 256], 1)

    disc = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (31, 31))
    cv2.filter2D(dst, -1, disc, dst)

    ret, thresh = cv2.threshold(dst, 150, 255, cv2.THRESH_BINARY)

    # thresh = cv2.dilate(thresh, None, iterations=5)

    thresh = cv2.merge((thresh, thresh, thresh))

    return cv2.bitwise_and(frame, thresh)


def centroid(max_contour):
    moment = cv2.moments(max_contour)
    if moment['m00'] != 0:
        cx = int(moment['m10'] / moment['m00'])
        cy = int(moment['m01'] / moment['m00'])
        return cx, cy
    else:
        return None


def farthest_point(defects, contour, centroid):
    if defects is not None and centroid is not None:
        s = defects[:, 0][:, 0]
        cx, cy = centroid

        x = np.array(contour[s][:, 0][:, 0], dtype=np.float)
        y = np.array(contour[s][:, 0][:, 1], dtype=np.float)

        xp = cv2.pow(cv2.subtract(x, cx), 2)
        yp = cv2.pow(cv2.subtract(y, cy), 2)
        dist = cv2.sqrt(cv2.add(xp, yp))

        dist_max_i = np.argmax(dist)

        if dist_max_i < len(s):
            farthest_defect = s[dist_max_i]
            farthest_point = tuple(contour[farthest_defect][0])
            return farthest_point
        else:
            return None


def draw_circles(frame, traverse_point):
    if traverse_point is not None:
        for i in range(len(traverse_point)):
            #int(5 - (5 * i * 3) / 100)
            cv2.circle(frame, traverse_point[i], 5, [0, 255, 255], -1)


def manage_image_opr(frame, hand_hist, points, draw_mode):
    hist_mask_image = hist_masking(frame, hand_hist)
    # hsvImage = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    
    u_hue = cv2.getTrackbarPos("Upper Hue",
                               "Color detectors")
    u_saturation = cv2.getTrackbarPos("Upper Saturation",
                                      "Color detectors")
    u_value = cv2.getTrackbarPos("Upper Value",
                                 "Color detectors")
    l_hue = cv2.getTrackbarPos("Lower Hue",
                               "Color detectors")
    l_saturation = cv2.getTrackbarPos("Lower Saturation",
                                      "Color detectors")
    l_value = cv2.getTrackbarPos("Lower Value",
                                 "Color detectors")
    Upper_hsv = np.array([u_hue, u_saturation, u_value])
    Lower_hsv = np.array([l_hue, l_saturation, l_value])
        
    kernel = np.ones((5, 5), np.uint8)
    Mask = cv2.inRange(hsv, Lower_hsv, Upper_hsv)
    Mask = cv2.erode(Mask, kernel, iterations = 1)
    Mask = cv2.morphologyEx(Mask, cv2.MORPH_OPEN, kernel)
    Mask = cv2.dilate(Mask, kernel, iterations = 1)
    cv2.imshow("Mask", Mask)

        #cv2.drawContours(frame,cnts, max_index,(0, 230, 255),6)



    hist_mask_image = cv2.erode(hist_mask_image, None, iterations=2)
    hist_mask_image = cv2.dilate(hist_mask_image, None, iterations=2)

    hist_mask_image = cv2.GaussianBlur(hist_mask_image, (15,15), 0)

    #contour_list = contours(hist_mask_image)
    _, contour_list, _ = cv2.findContours(Mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    areas = [cv2.contourArea(c) for c in contour_list]
    max_index = -1
    max_cont = None
    areas = [cv2.contourArea(c) for c in contour_list]
    if len(contour_list) > 0:
        max_cont = max(contour_list, key=cv2.contourArea)
        max_index = np.argmax(areas)


    cnt_centroid = centroid(max_cont)
    cv2.circle(frame, cnt_centroid, 5, [255, 0, 255], -1)

    cv2.drawContours(frame,contour_list, max_index,(0, 230, 255),6)
    
    if max_cont is not None:
        hull = cv2.convexHull(max_cont, returnPoints=False)
        defects = cv2.convexityDefects(max_cont, hull)
        far_point = farthest_point(defects, max_cont, cnt_centroid)
        points[0].appendleft(far_point)
        #print("Centroid : " + str(cnt_centroid) + ", farthest Point : " + str(far_point))
        cv2.circle(frame, far_point, 5, [0, 0, 255], -1)
        if len(traverse_point) < 50:
            traverse_point.append(far_point)
        else:
            traverse_point.pop(0)
            traverse_point.append(far_point)

        if(not draw_mode):
            draw_circles(frame, traverse_point)


def main():
    global hand_hist
    is_hand_hist_created = False

    cv2.namedWindow("Color detectors")
    cv2.createTrackbar("Upper Hue", "Color detectors", 153, 180, setValues)
    cv2.createTrackbar("Upper Saturation", "Color detectors", 255, 255, setValues)
    cv2.createTrackbar("Upper Value", "Color detectors", 255, 255, setValues)
    cv2.createTrackbar("Lower Hue", "Color detectors", 64, 180, setValues)
    cv2.createTrackbar("Lower Saturation", "Color detectors", 72, 255, setValues)
    cv2.createTrackbar("Lower Value", "Color detectors", 49, 255, setValues)
    bpoints = [deque(maxlen = 1024)]
    gpoints = [deque(maxlen = 1024)]
    rpoints = [deque(maxlen = 1024)]
    ypoints = [deque(maxlen = 1024)]
   
    # These indexes will be used to mark position
    # of pointers in colour array
    blue_index = 0
    green_index = 0
    red_index = 0
    yellow_index = 0

    draw_mode = False
 
    colors = [(255, 0, 0), (0, 255, 0), 
              (0, 0, 255), (0, 255, 255)]
    colorIndex = 0
    kernel = np.ones((5, 5), np.uint8)

    paintWindow = np.zeros((471, 636, 3)) + 255
    
    cv2.namedWindow('Paint', cv2.WINDOW_AUTOSIZE)
    
    capture = cv2.VideoCapture(0)
    while capture.isOpened():
        pressed_key = cv2.waitKey(1)
        #Should use ret value for error detection.
        ret, frame = capture.read()
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        frame = cv2.flip(frame, 1)
        hsv = cv2.flip(hsv, 1)
        
        if pressed_key & 0xFF == ord('z'):
            is_hand_hist_created = True
            hand_hist = hand_histogram(frame)
        if pressed_key & 0xFF == ord('d'):
            bpoints = [deque(maxlen = 512)]
            gpoints = [deque(maxlen = 512)]
            rpoints = [deque(maxlen = 512)]
            ypoints = [deque(maxlen = 512)]
   
            blue_index = 0
            green_index = 0
            red_index = 0
            yellow_index = 0
   
            paintWindow[67:, :, :] = 255
        
        if pressed_key & 0xFF == ord('c'):
            draw_mode = not draw_mode

        if is_hand_hist_created:
            manage_image_opr(frame, hand_hist,bpoints,draw_mode)
            
            print(bpoints)
            points = [bpoints, gpoints, rpoints, ypoints]
            if(draw_mode):
                for i in range(len(points)):
                    for j in range(len(points[i])):
                        for k in range(1, len(points[i][j])):
                            if points[i][j][k - 1] is None or points[i][j][k] is None:
                                continue
                            cv2.line(frame, points[i][j][k - 1], points[i][j][k], colors[i], 2)
                            cv2.line(paintWindow, points[i][j][k - 1], points[i][j][k], colors[i], 2)
   

        else:
            frame = draw_rect(frame)


                

        cv2.imshow("Live Feed", rescale_frame(frame))

        if pressed_key == 27:
            break

    cv2.destroyAllWindows()
    capture.release()


if __name__ == '__main__':
    main()
