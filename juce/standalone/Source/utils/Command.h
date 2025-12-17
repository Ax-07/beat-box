// DrumBox/juce/standalone/Source/utils/Command.h

#pragma once
#include <array>
#include <atomic>
#include <cstdint>

/**
 * @brief Commande pour la communication thread-safe UI → Audio
 * 
 * Utilisé pour transmettre des commandes de l'UI thread vers l'audio thread
 * sans allocation mémoire ni lock bloquant.
 */
struct Command
{
    enum Type : uint8_t 
    { 
        ToggleStep,   ///< Active/désactive un step du séquenceur
        SetBpm,       ///< Change le tempo
        SetPlaying    ///< Démarre/arrête la lecture
    };
    
    Type type{};    ///< Type de commande
    int lane = 0;   ///< Index de la piste (0=Kick, 1=Snare, 2=Hat)
    int step = 0;   ///< Index du step (0-15)
    bool on = false; ///< État ON/OFF pour ToggleStep et SetPlaying
    float f = 0.0f;  ///< Valeur float pour SetBpm
    
    // Factory methods pour créer des commandes typées
    
    /**
     * @brief Crée une commande pour toggle un step
     * @param lane Index de la piste (0-2)
     * @param step Index du step (0-15)
     * @param on Nouvel état du step
     */
    static Command toggleStep(int lane, int step, bool on)
    {
        Command c;
        c.type = ToggleStep;
        c.lane = lane;
        c.step = step;
        c.on = on;
        return c;
    }
    
    /**
     * @brief Crée une commande pour changer le BPM
     * @param bpm Nouveau tempo en battements par minute
     */
    static Command setBpm(float bpm)
    {
        Command c;
        c.type = SetBpm;
        c.f = bpm;
        return c;
    }
    
    /**
     * @brief Crée une commande pour démarrer/arrêter la lecture
     * @param playing true pour démarrer, false pour arrêter
     */
    static Command setPlaying(bool playing)
    {
        Command c;
        c.type = SetPlaying;
        c.on = playing;
        return c;
    }
};

/**
 * @brief Queue SPSC (Single Producer Single Consumer) lock-free
 * 
 * Permet la communication thread-safe entre UI et audio sans allocation
 * ni lock bloquant. L'UI thread est le producteur, l'audio thread est
 * le consommateur.
 * 
 * @tparam N Capacité maximale de la queue (puissance de 2 recommandée)
 * 
 * @warning Cette queue n'est thread-safe que pour UN producteur et UN consommateur.
 *          Ne pas utiliser avec plusieurs threads producteurs ou consommateurs.
 */
template <size_t N>
class SpscQueue
{
public:
    SpscQueue() = default;
    ~SpscQueue() = default;
    
    /**
     * @brief Ajoute une commande dans la queue (côté producteur/UI)
     * @param c Commande à ajouter
     * @return true si l'ajout a réussi, false si la queue est pleine
     */
    bool push(const Command& c)
    {
        auto h = head.load(std::memory_order_relaxed);
        auto n = (h + 1) % N;
        if (n == tail.load(std::memory_order_acquire)) 
            return false; // Queue pleine
        
        buf[h] = c;
        head.store(n, std::memory_order_release);
        return true;
    }

    /**
     * @brief Retire une commande de la queue (côté consommateur/audio)
     * @param out Référence où stocker la commande lue
     * @return true si une commande a été lue, false si la queue est vide
     */
    bool pop(Command& out)
    {
        auto t = tail.load(std::memory_order_relaxed);
        if (t == head.load(std::memory_order_acquire)) 
            return false; // Queue vide
        
        out = buf[t];
        tail.store((t + 1) % N, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Vérifie si la queue est vide
     * @return true si la queue ne contient aucune commande
     */
    bool isEmpty() const
    {
        return tail.load(std::memory_order_acquire) == head.load(std::memory_order_acquire);
    }

private:
    std::array<Command, N> buf{};           ///< Buffer circulaire des commandes
    std::atomic<size_t> head{0};            ///< Index d'écriture (producteur)
    std::atomic<size_t> tail{0};            ///< Index de lecture (consommateur)
};
