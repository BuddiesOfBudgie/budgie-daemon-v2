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

    protected:
        DisplayConfigurationCalculation *createCalculation();

        QList<DisplayConfigurationBatch *> m_batches;
        std::optional<DisplayConfigurationCalculation> *m_calculation;
    };

    class DisplayConfigurationCalculation : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationCalculation(QObject *parent = nullptr);

        void addAction(DisplayConfigurationBatchAction *action);

        void calculate();

    protected:
        QRect *m_global_space;
        QList<DisplayConfigurationBatchAction *> m_actions;
        QList<DisplayConfigurationBatchAction *> m_buffered_actions;
        QMap<QString, QRect *> m_output_rects;
    };

    class DisplayConfigurationBatch : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationBatch(const QString &serial, DisplayConfigurationBatchType batch_type,
                                  QObject *parent = nullptr);

        DisplayConfigurationBatchType batchType() const;

    protected:
        QString m_serial;
        QList<DisplayConfigurationBatchAction *> m_actions;
        DisplayConfigurationBatchType m_batch_type;
    };

    class DisplayConfigurationBatchAction : public QObject {
        Q_OBJECT

    public:
        DisplayConfigurationBatchAction(DisplayConfigurationBatchActionType action_type, QObject *parent = nullptr);

        DisplayConfigurationBatchAction gamma(double placeholder_ignore_me, QObject *parent = nullptr);

        DisplayConfigurationBatchAction mirrorOf(QString *serial, QObject *parent = nullptr);

        DisplayConfigurationBatchAction mode(int width, int height, int refresh, QObject *parent = nullptr);

        DisplayConfigurationBatchAction
        setPositionAnchor(QString *serial, DisplayConfigurationHorizontalAnchor horizontal,
                          DisplayConfigurationVerticalAnchor vertical, QObject *parent = nullptr);

        DisplayConfigurationBatchAction scale(double scale, QObject *parent = nullptr);

        DisplayConfigurationBatchAction transform(wl_output_transform transform, QObject *parent = nullptr);


        DisplayConfigurationBatchActionType actionType() const;

        std::optional<QString *> getRelative() const;

        std::optional<double> getGamma() const;

        std::optional<int> getWidth() const;

        std::optional<int> getHeight() const;

        std::optional<int> getRefresh() const;

        std::optional<DisplayConfigurationHorizontalAnchor> getHorizontalAnchor() const;

        std::optional<DisplayConfigurationVerticalAnchor> getVerticalAnchor() const;

        std::optional<double> getScale() const;

        std::optional<wl_output_transform> getTransform() const;

    protected:
        DisplayConfigurationBatchActionType m_action_type;

        // Shared by mirrorOf and setAnchorTo
        std::optional<QString *> relative;

        // Gamma
        std::optional<double> m_gamma;

        // Mode
        std::optional<int> m_width;
        std::optional<int> m_height;
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
