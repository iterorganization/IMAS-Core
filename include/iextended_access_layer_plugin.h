// New interface class with the additional function
class IExtendedAccessLayerPlugin {
public:
    virtual ~IExtendedAccessLayerPlugin() {}
    virtual void begin_timerange_action(int pulseCtx, const char* dataobjectname, int mode, double tmin, double tmax, double dtime, int interp, int opCtx) = 0;
};