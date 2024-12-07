#pragma once

#include <QObject>
#include <optional>
#include <QMap>
#include <wayland-client-protocol.h>

namespace bd {
    class DisplayConfigurationBatchSystem;

    class DisplayConfigurationBatch;

    class DisplayConfigurationCalculation;

    class DisplayConfigurationBatchAction;

    enum DisplayConfigurationBatchType {
        Enable,
        Disable,
    };

    enum DisplayConfigurationBatchActionType {
        SetGamma,
        SetMirrorOf,
        SetMode,
        SetPositionAnchor,
        SetScale,
        SetTransform,
    };

    enum DisplayConfigurationVerticalAnchor {
        Above,
        Top,
        Middle,
        Bottom,
        Below
    };

    enum DisplayConfigurationHorizontalAnchor {
        Left,
        Right,
        Center,
    };

    class DisplayConfigurationBatchSystem : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationBatchSystem(QObject *parent = nullptr);

        void addBatch(DisplayConfigurationBatch *batch);

        void apply();

        void reset();

        void test();

    public slots:
        // Useful for tracking when an output is removed, removing any associated batch from calculation
        void outputRemoved(QString *serial);

    private:
        DisplayConfigurationCalculation *m_calculation;
    };

    class DisplayConfigurationCalculation : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationCalculation(QObject *parent = nullptr);

        void addBatch(DisplayConfigurationBatch *batch);

        void calculate();

        QList<DisplayConfigurationBatchAction *> getActions();

        QMap<QString, QRect *> getOutputRects();

        void removeBatch(QString *serial);

        void reset();

    protected:
        QRect *m_global_space;
        QList<DisplayConfigurationBatchAction *> m_actions;
        QMap<QString *, DisplayConfigurationBatch *> m_batches;
        QMap<QString *, QRect *> m_output_rects;
    };

    class DisplayConfigurationBatch : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationBatch(QString *serial, DisplayConfigurationBatchType batch_type,
                                  QObject *parent = nullptr);

        DisplayConfigurationBatchType batchType() const;

        void addAction(DisplayConfigurationBatchAction *action);

        QMap<DisplayConfigurationBatchActionType, DisplayConfigurationBatchAction *> getActions();

        DisplayConfigurationBatchType getBatchType() const;

        QString *getSerial() const;

    protected:
        QString *m_serial;
        QMap<DisplayConfigurationBatchActionType, DisplayConfigurationBatchAction *> m_actions;
        DisplayConfigurationBatchType m_batch_type;
    };

    class DisplayConfigurationBatchAction : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationBatchAction(DisplayConfigurationBatchActionType action_type, QString *serial,
                                        QObject *parent = nullptr);

        DisplayConfigurationBatchAction gamma(double placeholder_ignore_me, QString *serial, QObject *parent = nullptr);

        DisplayConfigurationBatchAction mirrorOf(QString *relative, QString *serial, QObject *parent = nullptr);

        DisplayConfigurationBatchAction mode(QSize *dimensions, int refresh, QString *serial,
                                             QObject *parent = nullptr);

        DisplayConfigurationBatchAction
        setPositionAnchor(QString *relative, DisplayConfigurationHorizontalAnchor horizontal,
                          DisplayConfigurationVerticalAnchor vertical, QString *serial, QObject *parent = nullptr);

        DisplayConfigurationBatchAction scale(double scale, QString *serial, QObject *parent = nullptr);

        DisplayConfigurationBatchAction transform(wl_output_transform transform, QString *serial,
                                                  QObject *parent = nullptr);


        DisplayConfigurationBatchActionType actionType() const;

        std::optional<QString *> getRelative() const;

        std::optional<double> getGamma() const;

        std::optional<QSize *> getDimensions() const;

        std::optional<int> getRefresh() const;

        std::optional<DisplayConfigurationHorizontalAnchor> getHorizontalAnchor() const;

        std::optional<DisplayConfigurationVerticalAnchor> getVerticalAnchor() const;

        std::optional<double> getScale() const;

        QString *getSerial() const;

        std::optional<wl_output_transform> getTransform() const;

    protected:
        DisplayConfigurationBatchActionType m_action_type;
        QSerial *m_serial;

        // Shared by mirrorOf and setAnchorTo
        std::optional<QString *> relative;

        // Gamma
        std::optional<double> m_gamma;

        // Mode
        std::optional<QSize *> m_dimensions;
        std::optional<int> m_refresh;

        // Set Position Anchor
        std::optional<DisplayConfigurationHorizontalAnchor> m_horizontal;
        std::optional<DisplayConfigurationVerticalAnchor> m_vertical;

        // Scale
        std::optional<double> m_scale;

        // Transform
        std::optional<wl_output_transform> m_transform;
    };
}
