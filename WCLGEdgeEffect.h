// WCLGEdgeEffect.h
// WCG edge blur: variable blur + a lightweight edge scrim mask.
// It does not apply a content-opacity fade to the message scroll view.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, WCLGEdgeEffectEdge) {
    WCLGEdgeEffectEdgeTop = 0,
    WCLGEdgeEffectEdgeBottom = 1,
};

@interface WCLGEdgeEffectView : UIView

@property (nonatomic, assign, getter=isBlurEnabled) BOOL blurEnabled;

- (void)updateWithContentColor:(UIColor *)contentColor
                          blur:(BOOL)blur
                         alpha:(CGFloat)alpha
                          rect:(CGRect)rect
                          edge:(WCLGEdgeEffectEdge)edge
                      edgeSize:(CGFloat)edgeSize
                    blurRadius:(CGFloat)blurRadius;

- (void)updateWithContentColor:(UIColor *)contentColor
                          blur:(BOOL)blur
                         alpha:(CGFloat)alpha
                          rect:(CGRect)rect
                          edge:(WCLGEdgeEffectEdge)edge
                     edgeInset:(CGFloat)edgeInset
              transitionLength:(CGFloat)transitionLength
                    blurRadius:(CGFloat)blurRadius;

@end

#ifdef __cplusplus
extern "C" {
#endif

UIImage *_Nullable WCLGEdgeGradientMaskImage(CGFloat baseHeight, BOOL inverted);
BOOL WCLGEdgeVariableBlurIsAvailable(void);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END
